#include "src/search.h"
#include "src/cli.h"
#include "src/utils.h"
#include "src/criteria.h"
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <stdatomic.h>

#include "src/cli.c"
#include "src/criteria.c"
#include "src/output.c"
#include "src/pattern.c"
#include "src/platform.c"
#include "src/search.c"
#include "src/thread_pool.c"
#include "src/utils.c"
#include "src/version.c"

static bool search_progress(size_t processed_files, size_t queued_dirs, size_t total_results, void *user_data) {
    cli_options_t *options = (cli_options_t*)user_data;
    static _Atomic size_t last_update = 0;
    size_t prev = atomic_load(&last_update);
    if (processed_files >= prev + 1000 || queued_dirs == 0) {
        if (options && options->show_stats) {
            thread_pool_stats_t thread_stats;
            if (get_last_search_thread_stats(&thread_stats)) {
                fprintf(stderr, "\rProcessed: %zu files, Queued: %zu dirs, Found: %zu results | Pool: %zu active, %zu queued",
                        processed_files, queued_dirs, total_results,
                        thread_stats.active_threads, thread_stats.queued_work_items);
            } else {
                fprintf(stderr, "\rProcessed: %zu files, Queued: %zu dirs, Found: %zu results",
                        processed_files, queued_dirs, total_results);
            }
        } else {
            fprintf(stderr, "\rProcessed: %zu files, Queued: %zu dirs, Found: %zu results",
                    processed_files, queued_dirs, total_results);
        }
        atomic_store(&last_update, processed_files);
    }
    return true;
}

int main(int argc, char *argv[]) {
    search_criteria_t criteria;
    cli_options_t options = {0};
    search_result_t *results = NULL;
    size_t result_count = 0;
    int exit_code = 0;

    if (parse_command_line(argc, argv, &criteria, &options) != 0) {
        if (!options.show_help && !options.show_version) {
            fprintf(stderr, "Error: Invalid command line arguments\n");
            print_usage(argv[0]);
            exit_code = 1;
        }
        goto cleanup;
    }

    if (options.show_help) {
        print_usage(argv[0]);
        goto cleanup;
    }

    if (options.show_version) {
        print_version();
        goto cleanup;
    }

    if (!criteria_validate(&criteria)) {
        fprintf(stderr, "Error: Invalid search criteria\n");
        exit_code = 1;
        goto cleanup;
    }

    platform_dir_iter_t *test_dir = platform_opendir(criteria.root_path);
    if (!test_dir) {
        fprintf(stderr, "Error: Root directory '%s' does not exist or cannot be accessed\n", criteria.root_path);
        exit_code = 1;
        goto cleanup;
    }
    platform_closedir(test_dir);

    fprintf(stderr, "Searching in '%s' for '%s'...\n", criteria.root_path, criteria.search_term ? criteria.search_term : "*");

    int search_result = search_files_advanced(&criteria, &results, &result_count, NULL, NULL, search_progress, &options);

    fprintf(stderr, "\n");

    if (search_result == -2) {
        fprintf(stderr,
                "Warning: Search timed out after %" PRIu64 " ms\n",
                (uint64_t)criteria.timeout_ms);
    } else if (search_result != 0) {
        fprintf(stderr, "Error: Search operation failed\n");
        exit_code = 1;
        goto cleanup;
    }

    if (output_results(results, result_count, &options) != 0) {
        fprintf(stderr, "Error: Failed to output results\n");
        exit_code = 1;
        goto cleanup;
    }

cleanup:
    free_search_results(results);
    criteria_cleanup(&criteria);
    return exit_code;
}
