#ifndef OUTPUT_H
#define OUTPUT_H

#include "search.h"
#include <stdio.h>

typedef enum {
    OUTPUT_FORMAT_TEXT,
    OUTPUT_FORMAT_JSON
} output_format_t;

int output_search_results(FILE *fp, const search_result_t *results, size_t count, output_format_t format);

#endif
