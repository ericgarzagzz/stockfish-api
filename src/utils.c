#include "utils.h"

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *stream) {
	size_t written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;
}

int ensure_directory_exists(const char *path) {
	struct stat st = {0};

	if (stat(path, &st) == -1) {
		if (mkdir(path, 0700) == -1) {
			fprintf(stderr, "Failed to create directory %s: %s\n", path, strerror(errno));
			return -1;
		}
	}
	return 0;
}

bool check_file_accessible(const char *path) {
	struct stat st = {0};
	return stat(path, &st) == 0;
}
