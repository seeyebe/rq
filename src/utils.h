#ifndef UTILS_H
#define UTILS_H

#include <windows.h>
#include <stdbool.h>
#include <stddef.h>

int parse_date_string(const char *date_str, FILETIME *file_time);
void format_filetime_iso(const FILETIME *file_time, char *buffer, size_t buffer_size);
bool path_exists(const char *path);

#endif
