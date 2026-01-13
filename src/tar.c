#include "tar.h"
#include "utils.h"
#include <string.h>

bool extract_tar_item(struct posix_header *hdr, FILE *tar_file) {
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
		return false;
	}

	size_t blocks = (file_size + 511) / 512;
	for (size_t i = 0; i < blocks; i++) {
		char buf[TAR_BLOCK_SIZE];
		if (fread(buf, TAR_BLOCK_SIZE, 1, tar_file) != 1) {
			fprintf(stderr, "Unexpected EOF reading file data\n");
			return false;
		}

		// TODO: Write buffer to output file or create a directory
	}

	return true;
}

int extract_tar(const char* path) {
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

		if (!extract_tar_item(&hdr, f)) {
			fprintf(stderr, "Failed to extract %s\n", hdr.name);
			fclose(f);
			return -1;
		}
	}
	
	fclose(f);

	return 0;
}
