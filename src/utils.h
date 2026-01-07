#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *stream);
int ensure_directory_exists(const char *path);
bool check_file_accessible(const char *path);

#endif
