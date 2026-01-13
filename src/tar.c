#include "tar.h"
#include "utils.h"
#include <linux/limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdlib.h>

bool extract_tar_item(struct posix_header *hdr, FILE *tar_file, const char *rootdir) {
	size_t output_path_length = strlen(rootdir) + strlen(hdr->name) + 1;
	if (output_path_length > PATH_MAX) {
		fprintf(stderr, "The output path cannot be longer than %d\n", PATH_MAX);
		return false;
	}
	char *output_path = malloc(output_path_length);
	if (!output_path) {
		fprintf(stderr, "Failed to allocate memory for output path\n");
		return false;
	}
	snprintf(output_path, output_path_length, "%s%s", rootdir, hdr->name);

	/* Switch for type flag matching */
	if (hdr->typeflag == REGTYPE || hdr->typeflag == AREGTYPE) {
		printf("Extracting file: %s...\n", hdr->name);
	} else if (hdr->typeflag == LNKTYPE) {
		printf("Extracting symbolic link: %s...\n", hdr->linkname);
	} else if (hdr->typeflag == SYMTYPE) {
		printf("Extracting symbolic link: %s...\n", hdr->linkname);
	} else if (hdr->typeflag == CHRTYPE || hdr->typeflag == BLKTYPE) {
		printf("Extracting special file: %s...\n", hdr->name);
	} else if (hdr->typeflag == DIRTYPE) {
		printf("Extracting directory: %s...\n", hdr->name);
	} else if (hdr->typeflag == FIFOTYPE) {
		printf("Extracting FIFO special file: %s...\n", hdr->name);
	} else if (hdr->typeflag == CONTTYPE) {
		printf("Extracting contiguous file: %s...\n", hdr->name);
	}

	ulong file_size;
	bool valid_file_size = parse_octal(hdr->size, 12, &file_size);

	if (!valid_file_size) {
		fprintf(stderr, "Failed to read the file size of one of the files inside the tar ball\n");
		free(output_path);
		return false;
	}


	if (hdr->typeflag == DIRTYPE) {
		printf("Creating directory...\n");
		ensure_directory_exists(output_path); // TODO: Get mode from tar header
		free(output_path);
		return true;
	}
	FILE* output_file;
	output_file = fopen(output_path, "ab");
	if (!output_file) {
		fprintf(stderr, "Failed to open or create file at path %s\n", output_path);
		free(output_path);
		return false;
	}

	size_t blocks = (file_size + 511) / 512;
	for (size_t i = 0; i < blocks; i++) {
		char buf[TAR_BLOCK_SIZE];
		if (fread(buf, TAR_BLOCK_SIZE, 1, tar_file) != 1) {
			fprintf(stderr, "Unexpected EOF reading file data\n");
			fclose(output_file);
			free(output_path);
			return false;
		}

		// TODO: Write buffer to output file or create a directory	
		if (fwrite(buf, sizeof(buf), 1, output_file) != 1) {
			fprintf(stderr, "Failed to write extracted data into file\n");
			fclose(output_file);
			free(output_path);
			return false;
		}
	}

	fclose(output_file);

	free(output_path);

	return true;
}

int extract_tar(const char* path, const char *rootdir) {
	assert(rootdir[strlen(rootdir) - 1] == '/');
	printf("Extracting into rootdir: %s", rootdir);

	if (ensure_directory_exists(rootdir) != 0) {
		fprintf(stderr, "Failed to access the root directory: %s\n", rootdir);
		return -1;
	}


	if (!check_file_accessible(path)) {
		fprintf(stderr, "Failed to find tar ball at %s\n", path);
		return -1;
	}

	FILE* f;
	f = fopen(path, "rb");
	if (!f) {
		fprintf(stderr, "Something went wrong while trying to read at %s\n", path);
		return -1;
	}

	struct posix_header hdr;
	while (fread(&hdr, TAR_BLOCK_SIZE, 1, f)) {
		if (hdr.name[0] == '\0') {
			break;
		}

		if (!extract_tar_item(&hdr, f, rootdir)) {
			fprintf(stderr, "Failed to extract %s\n", hdr.name);
			fclose(f);
			return -1;
		}
	}
	
	fclose(f);

	return 0;
}
