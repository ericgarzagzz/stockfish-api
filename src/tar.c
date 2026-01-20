#include "tar.h"
#include "arena.h"
#include "utils.h"
#include <assert.h>
#include <linux/limits.h>
#include <regex.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

bool extract_tar_item(Arena *arena, struct posix_header *hdr, FILE *tar_file,
                      const char *rootdir, const char *regex_pattern) {
  char *output_path = arena_sprintf(arena, "%s%s", rootdir, hdr->name);

  ulong file_size;
  bool valid_file_size = parse_octal(hdr->size, 12, &file_size);

  if (!valid_file_size) {
    fprintf(stderr, "Failed to read the file size of one of the files inside "
                    "the tar ball\n");
    return false;
  }

  /* Filter by regex pattern if provided (directories are always extracted) */
  if (strlen(regex_pattern) > 0 && hdr->typeflag != DIRTYPE) {
    regex_t regex;
    int reti = regcomp(&regex, regex_pattern, REG_EXTENDED);
    if (reti) {
      fprintf(stderr, "Could not compile regex pattern\n");
      return false;
    }

    reti = regexec(&regex, hdr->name, 0, NULL, 0);
    regfree(&regex);

    if (reti == REG_NOMATCH) {
      /* Skip this file's data blocks if it's a regular file */
      if (hdr->typeflag == REGTYPE || hdr->typeflag == AREGTYPE) {
        size_t blocks = (file_size + 511) / 512;
        fseek(tar_file, blocks * TAR_BLOCK_SIZE, SEEK_CUR);
      }
      return true;
    } else if (reti) {
      fprintf(stderr, "Regex match failed\n");
      return false;
    }
  }

  /* Switch for type flag matching */
  if (hdr->typeflag == REGTYPE || hdr->typeflag == AREGTYPE) {
    printf("Extracting file: %s...\n", hdr->name);
    FILE *output_file;
    output_file = fopen(output_path, "ab");
    if (!output_file) {
      fprintf(stderr, "Failed to open or create file at path %s\n",
              output_path);
      return false;
    }

    size_t blocks = (file_size + 511) / 512;
    for (size_t i = 0; i < blocks; i++) {
      char buf[TAR_BLOCK_SIZE];
      if (fread(buf, TAR_BLOCK_SIZE, 1, tar_file) != 1) {
        fprintf(stderr, "Unexpected EOF reading file data\n");
        fclose(output_file);
        return false;
      }

      // TODO: Write buffer to output file or create a directory
      if (fwrite(buf, sizeof(buf), 1, output_file) != 1) {
        fprintf(stderr, "Failed to write extracted data into file\n");
        fclose(output_file);
        return false;
      }
    }

    fclose(output_file);
  } else if (hdr->typeflag == LNKTYPE) {
    printf("Extracting hard link: %s link to %s...\n", hdr->name,
           hdr->linkname);
    char *original_file_path =
        arena_sprintf(arena, "%s%s", rootdir, hdr->linkname);

    // TODO: Schedule the creation of the hard link
    if (ensure_file_exists(original_file_path) != 0) {
      fprintf(
          stderr,
          "Failed to create a temporary original file %s for hard link %s\n",
          hdr->linkname, hdr->name);
      return false;
    }

    // The hard link file needs to be removed in order for link() to overwrite
    if (check_file_accessible(output_path)) {
      if (remove(output_path) != 0) {
        fprintf(stderr, "Failed to overwrite output file\n");
        return false;
      }
    }

    if (link(original_file_path, output_path) != 0) {
      fprintf(stderr, "Failed to extract hard link\n");
      return false;
    }
  } else if (hdr->typeflag == SYMTYPE) {
    printf("Extracting soft link: %s -> %s...\n", hdr->name, hdr->linkname);
    // The symbolic link file needs to be removed in order for symlink() to
    // overwrite
    if (check_file_accessible(output_path)) {
      if (remove(output_path) != 0) {
        fprintf(stderr, "Failed to overwrite output file\n");
        return false;
      }
    }
    if (symlink(hdr->linkname, output_path) != 0) {
      fprintf(stderr, "Failed to extract soft link\n");
      return false;
    }
  } else if (hdr->typeflag == CHRTYPE || hdr->typeflag == BLKTYPE) {
    printf("Extracting special file: %s...\n", hdr->name);
  } else if (hdr->typeflag == DIRTYPE) {
    printf("Extracting directory: %s...\n", hdr->name);
    // TODO: Get mode from tar header
    if (ensure_directory_exists(output_path) != 0) {
      fprintf(stderr, "Failed to extract directory\n");
      return false;
    }
    return true;
  } else if (hdr->typeflag == FIFOTYPE) {
    printf("Extracting FIFO special file: %s...\n", hdr->name);
  } else if (hdr->typeflag == CONTTYPE) {
    printf("Extracting contiguous file: %s...\n", hdr->name);
  }

  return true;
}

int extract_tar(Arena *arena, const char *path, const char *rootdir,
                const char *regex_pattern) {
  assert(rootdir[strlen(rootdir) - 1] == '/');

  printf("Extracting into rootdir: %s\n", rootdir);

  if (ensure_directory_exists(rootdir) != 0) {
    fprintf(stderr, "Failed to access the root directory: %s\n", rootdir);
    return -1;
  }

  if (!check_file_accessible(path)) {
    fprintf(stderr, "Failed to find tar ball at %s\n", path);
    return -1;
  }

  FILE *f;
  f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "Something went wrong while trying to read at %s\n", path);
    return -1;
  }

  struct posix_header hdr;
  while (fread(&hdr, TAR_BLOCK_SIZE, 1, f)) {
    arena_reset(arena);
    if (hdr.name[0] == '\0') {
      break;
    }

    if (!extract_tar_item(arena, &hdr, f, rootdir, regex_pattern)) {
      fprintf(stderr, "Failed to extract %s\n", hdr.name);
      fclose(f);
      return -1;
    }
  }

  fclose(f);

  return 0;
}
