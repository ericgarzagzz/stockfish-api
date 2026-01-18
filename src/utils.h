#ifndef UTILS_H
#define UTILS_H

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>

size_t write_cb(void *ptr, size_t size, size_t nmemb, void *stream);
int ensure_directory_exists(const char *path);
int ensure_file_exists(const char *path);
bool check_file_accessible(const char *path);
bool parse_octal(const char *s, size_t size, ulong *value);

#endif
