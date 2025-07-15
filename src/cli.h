#ifndef CLI_H
#define CLI_H

#include "criteria.h"
#include "search.h"
#include <stdbool.h>

typedef struct cli_options {
    char *output_file;
    bool json_output;
    bool show_help;
    bool show_version;
    bool show_stats;
} cli_options_t;

int parse_command_line(int argc, char *argv[], search_criteria_t *criteria, cli_options_t *options);
void print_usage(const char *program_name);
void print_version(void);
int output_results(const search_result_t *results, size_t count, const cli_options_t *options, const search_criteria_t *criteria);

#endif
