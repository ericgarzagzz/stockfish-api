#include "utils.h"
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
  size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
  return written;
}

int ensure_directory_exists(const char *path) {
  struct stat st = {0};

  if (stat(path, &st) == -1) {
    if (mkdir(path, 0700) == -1) {
      fprintf(stderr, "Failed to create directory %s: %s\n", path,
              strerror(errno));
      return -1;
    }
  }
  return 0;
}

int ensure_file_exists(const char *path) {
  struct stat st = {0};

  if (stat(path, &st) == -1) {
    // Create an empty file
    FILE *f = fopen(path, "w");
    if (!f) {
      fprintf(stderr, "Failed to create empty file %s: %s\n", path,
              strerror(errno));
      return -1;
    }

    fclose(f);
  }
  return 0;
}

bool check_file_accessible(const char *path) {
  struct stat st = {0};
  return stat(path, &st) == 0;
}

int make_file_executable(const char *path) {
  int fd = open(path, O_RDONLY);
  if (fd == -1) {
    fprintf(stderr, "Failed open file to make executable at %s: %s\n", path,
            strerror(errno));
    return -1;
  }

  struct stat st;
  if (fstat(fd, &st) == -1) {
    fprintf(stderr, "Failed to stat output file at %s: %s\n", path,
            strerror(errno));
    close(fd);
    return -1;
  }

  mode_t new_mode = st.st_mode | S_IXUSR;
  if (fchmod(fd, new_mode) == -1) {
    fprintf(stderr, "Failed to change output file to executable at %s: %s\n",
            path, strerror(errno));
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

bool parse_octal(const char *s, size_t size, ulong *value) {
  size_t ofs;

  *value = 0;
  for (ofs = 0; ofs < size; ofs++) {
    char c = s[ofs];
    if (c >= '0' && c <= '7') {
      if (*value > ULONG_MAX / 8) {
        /* Overflow. */
        return false;
      }
      *value = c - '0' + *value * 8;
    } else if (c == ' ' || c == '\0') {
      /* End of field, but disallow completely empty
         fields. */
      return ofs > 0;
    } else {
      /* Bad character. */
      return false;
    }
  }

  /* Field did not end in space or null byte. */
  return false;
}
