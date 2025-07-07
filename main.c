#define _CRT_SECURE_NO_WARNINGS

#include "src/search.h"
#include "src/cli.h"
#include "src/utils.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[]) {
    search_criteria_t criteria;
    cli_options_t options;
    search_result_t *results = NULL;
    size_t result_count = 0;

    if (parse_command_line(argc, argv, &criteria, &options) != 0) {
        if (!options.show_help && !options.show_version) {
            fprintf(stderr, "Error: Invalid command line arguments\n");
            print_usage(argv[0]);
            return 1;
        }
    }

    if (options.show_help) {
        print_usage(argv[0]);
        return 0;
    }

    if (options.show_version) {
        print_version();
        return 0;
    }

    if (!path_exists(criteria.root_path)) {
        fprintf(stderr, "Error: Root directory '%s' does not exist\n", criteria.root_path);
        return 1;
    }

    int search_result = search_files_fast(&criteria, &results, &result_count);
    if (search_result != 0) {
        fprintf(stderr, "Error: Search operation failed\n");
        free_search_results(results);
        return 1;
    }

    if (output_results(results, result_count, &options) != 0) {
        fprintf(stderr, "Error: Failed to output results\n");
        free_search_results(results);
        return 1;
    }

    free_search_results(results);
    return 0;
}
