#ifndef PREVIEW_H
#define PREVIEW_H

#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum {
    RQ_FILE_TYPE_TEXT,
    RQ_FILE_TYPE_IMAGE,
    RQ_FILE_TYPE_VIDEO,
    RQ_FILE_TYPE_AUDIO,
    RQ_FILE_TYPE_ARCHIVE,
    RQ_FILE_TYPE_UNKNOWN
} rq_file_type_t;

int preview_text_file(const char *filepath, size_t max_lines, FILE *output);

rq_file_type_t detect_file_type(const char *filepath);

const char* file_type_to_string(rq_file_type_t type);

bool is_text_file(const char *filepath);

int preview_file_summary(const char *filepath, FILE *output);

#endif
