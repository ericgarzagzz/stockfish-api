#include <stddef.h>
#include <stdio.h>
#include <curl/curl.h>
#include <sys/types.h>
#include "utils.h"
#include "tar.h"

static const char *stockfish_tar_filename = ".cache/stockfish.tar";
static const char *stockfish_exec_regex_pattern = "stockfish/stockfish-ubuntu-x86-64";

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

	const char *rootdir = ".cache/";
	if (extract_tar(stockfish_tar_filename, rootdir, stockfish_exec_regex_pattern) != 0) {
		fprintf(stderr, "Failed extracting stockfish tarball at %s\n", stockfish_tar_filename);
		return -1;
	}

	return 0;
}
