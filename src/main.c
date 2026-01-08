#include <stddef.h>
#include <stdio.h>
#include <curl/curl.h>
#include <sys/types.h>
#include "utils.h"

static const char *stockfish_tar_filename = ".cache/stockfish.tar";

/* tar Header Block, from POSIX 1003.1-1990.  */

/* POSIX header.  */

struct posix_header
{                              /* byte offset */
	char name[100];               /*   0 */
	char mode[8];                 /* 100 */
	char uid[8];                  /* 108 */
	char gid[8];                  /* 116 */
	char size[12];                /* 124 */
	char mtime[12];               /* 136 */
	char chksum[8];               /* 148 */
	char typeflag;                /* 156 */
	char linkname[100];           /* 157 */
	char magic[6];                /* 257 */
	char version[2];              /* 263 */
	char uname[32];               /* 265 */
	char gname[32];               /* 297 */
	char devmajor[8];             /* 329 */
	char devminor[8];             /* 337 */
	char prefix[155];             /* 345 */
	/* 500 */
};

int download_stockfish_executable() {
	CURLcode result;
	CURL* curl;

	result = curl_global_init(CURL_GLOBAL_ALL);
	if (result) {
		fprintf(stderr, "Failed to initialize curl library\n");
		return (int)result;
	}

	curl = curl_easy_init();
	if (!curl) {
		fprintf(stderr, "Failed to initialize curl handle\n");
		curl_global_cleanup();
		return CURLE_FAILED_INIT;
	}

	FILE* stockfish_tar;

	// TODO: Support Windows and MacOS as well (once cross-platform compilation is implemented)
	char* download_url = "https://github.com/official-stockfish/Stockfish/releases/download/sf_17.1/stockfish-ubuntu-x86-64.tar";
	curl_easy_setopt(curl, CURLOPT_URL, download_url);
	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);	

	if (ensure_directory_exists(".cache") != 0) {
		curl_easy_cleanup(curl);
		curl_global_cleanup();
		return -1;
	}

	stockfish_tar = fopen(stockfish_tar_filename, "wb");
	if (stockfish_tar) {
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, stockfish_tar);

		result = curl_easy_perform(curl);

		fclose(stockfish_tar);
	}

	curl_easy_cleanup(curl);

	curl_global_cleanup();

	return (int)result;
}

bool extract_file_data(FILE *f, const char *name, size_t file_size, size_t block_size) {
	printf("Extracting %s...\n", name);

	size_t blocks = (file_size + 511) / 512;
	for (size_t i = 0; i < blocks; i++) {
		char buf[block_size];
		if (fread(buf, block_size, 1, f) != 1) {
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
	}

	size_t block_size = 512;

	struct posix_header hdr;
	while (fread(&hdr, block_size, 1, f)) {
		if (hdr.name[0] == '\0') {
			break;
		}

		ulong file_size;
		bool valid_file_size = parse_octal(hdr.size, 12, &file_size);

		if (!valid_file_size) {
			fprintf(stderr, "Failed to read the file size of one of the files inside the tar ball\n");
			fclose(f);
			return -1;
		}

		if (!extract_file_data(f, hdr.name, file_size, block_size)) {
			fprintf(stderr, "Failed to extract %s\n", hdr.name);
			fclose(f);
			return -1;
		}

	}
	
	fclose(f);

	return 0;
}

int main(int argc, char** argv) {
	printf("Stockfish API called with %d arguments.\n", argc);
	for (int i = 0; i < argc; i++) {
		printf("Argument %d: %s\n", (i+1), argv[i]);
	}

	printf("Setting up stockfish...\n");
	if (!check_file_accessible(stockfish_tar_filename)) {
		printf("Downloading stockfish...\n");

		int download_rc = download_stockfish_executable();

		if (download_rc != CURLE_OK) {
			return 1;
		}

		printf("Stockfish has been downloaded.\n");
	}

	extract_tar(stockfish_tar_filename);

	return 0;
}
