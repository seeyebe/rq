#include "cli.h"
#include "criteria.h"
#include "utils.h"
#include "output.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void init_options(cli_options_t *options) {
    memset(options, 0, sizeof(cli_options_t));
}

static int parse_date_arg(const char *arg, FILETIME *file_time) {
    return parse_date_string(arg, file_time);
}

void print_usage(const char *program_name) {
    printf("snub - fast file search tool for Windows\n\n");
    printf("Usage: %s <directory> <pattern> [OPTIONS]\n\n", program_name);

    printf("Arguments:\n");
    printf("  <directory>         The directory to search in\n");
    printf("  <pattern>           Search pattern (use --glob for wildcards)\n\n");

    printf("Search Options:\n");
    printf("  -c, --case          Case-sensitive search\n");
    printf("  -g, --glob          Enable glob patterns (* ? [] {})\n");
    printf("  -H, --include-hidden    Include hidden files and directories\n");
    printf("  -L, --follow-symlinks   Follow symbolic links\n");
    printf("      --no-skip       Don't skip common directories (node_modules, .git, etc.)\n\n");

    printf("Filters:\n");
    printf("  -e, --ext <list>    Filter by file extensions (comma-separated)\n");
    printf("      --min <size>    Minimum file size (supports K, M, G, T suffixes)\n");
    printf("      --max <size>    Maximum file size (supports K, M, G, T suffixes)\n");
    printf("      --size <size>   Exact file size, or +size (larger), -size (smaller)\n");
    printf("      --after <date>  Files modified after date (YYYY-MM-DD)\n");
    printf("      --before <date> Files modified before date (YYYY-MM-DD)\n");
    printf("  -d, --max-depth <n> Maximum recursion depth (0 = no recursion, default = unlimited)\n");
    printf("      --max-results <n>   Maximum number of results (0 = unlimited)\n\n");

    printf("Performance:\n");
    printf("  -j, --threads <n>   Number of worker threads (0 = auto)\n");
    printf("      --timeout <ms>  Search timeout in milliseconds\n");
    printf("      --stats         Show real-time thread pool statistics\n\n");

    printf("Output:\n");
    printf("      --out <file>    Write output to file\n");
    printf("      --json          Output results as JSON\n\n");

    printf("General:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -V, --version       Show version information\n\n");

    printf("Examples:\n");
    printf("  Search for all PNG files:\n");
    printf("    %s D:\\ \"*.png\" --glob\n\n", program_name);
    printf("  Find documents larger than 1MB:\n");
    printf("    %s . document --min 1M --ext pdf,docx\n\n", program_name);
    printf("  Find files smaller than 100KB:\n");
    printf("    %s . \"\" --size -100K --ext txt\n\n", program_name);
    printf("  Case-sensitive search with thread monitoring:\n");
    printf("    %s C:\\ \"Config\" --case --stats --threads 8\n\n", program_name);

    printf("For glob patterns: * (any chars), ? (single char), [abc] (char set), {jpg,png} (alternatives)\n");
}

void print_version(void) {
    printf("%s\n", get_version_string());
    printf("Copyright (c) 2025. Open source under MIT license.\n");
}

int parse_command_line(int argc, char *argv[], search_criteria_t *criteria, cli_options_t *options) {
    criteria_init(criteria);
    init_options(options);

    if (argc < 2) {
        return -1;
    }

    if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        options->show_help = true;
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0) {
        options->show_version = true;
        return 0;
    }

    if (argc < 3) {
        return -1;
    }

    criteria->root_path = _strdup(argv[1]);
    criteria->search_term = _strdup(argv[2]);

    if (!criteria->root_path || !criteria->search_term) {
        criteria_cleanup(criteria);
        return -1;
    }

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--case") == 0 || strcmp(argv[i], "-c") == 0) {
            criteria->case_sensitive = true;
        } else if (strcmp(argv[i], "--glob") == 0 || strcmp(argv[i], "-g") == 0) {
            criteria->use_glob = true;
        } else if (strcmp(argv[i], "--no-skip") == 0) {
            criteria->skip_common_dirs = false;
        } else if (strcmp(argv[i], "--follow-symlinks") == 0 || strcmp(argv[i], "-L") == 0) {
            criteria->follow_symlinks = true;
        } else if (strcmp(argv[i], "--include-hidden") == 0 || strcmp(argv[i], "-H") == 0) {
            criteria->include_hidden = true;
        } else if (strcmp(argv[i], "--ext") == 0 || strcmp(argv[i], "-e") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            if (!criteria_parse_extensions(criteria, argv[i])) {
                criteria_cleanup(criteria);
                return -1;
            }
        } else if (strcmp(argv[i], "--min") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            if (parse_size_arg(argv[i], &criteria->min_size) != 0) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->has_min_size = true;
        } else if (strcmp(argv[i], "--max") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            if (parse_size_arg(argv[i], &criteria->max_size) != 0) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->has_max_size = true;
        } else if (strcmp(argv[i], "--size") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            uint64_t size;
            char operator;
            if (parse_size_with_operator(argv[i], &size, &operator) != 0) {
                criteria_cleanup(criteria);
                return -1;
            }
            if (operator == '+') {
                criteria->min_size = size + 1;
                criteria->has_min_size = true;
            } else if (operator == '-') {
                criteria->max_size = size - 1;
                criteria->has_max_size = true;
            } else {
                criteria->exact_size = size;
                criteria->has_exact_size = true;
            }
        } else if (strcmp(argv[i], "--after") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            if (parse_date_arg(argv[i], &criteria->after_time) != 0) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->has_after_time = true;
        } else if (strcmp(argv[i], "--before") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            if (parse_date_arg(argv[i], &criteria->before_time) != 0) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->has_before_time = true;
        } else if (strcmp(argv[i], "--max-results") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->max_results = (size_t)strtoull(argv[i], NULL, 10);
        } else if (strcmp(argv[i], "--max-depth") == 0 || strcmp(argv[i], "-d") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->max_depth = (size_t)strtoull(argv[i], NULL, 10);
        } else if (strcmp(argv[i], "--threads") == 0 || strcmp(argv[i], "-j") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->max_threads = (size_t)strtoull(argv[i], NULL, 10);
        } else if (strcmp(argv[i], "--timeout") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->timeout_ms = (DWORD)strtoul(argv[i], NULL, 10);
        } else if (strcmp(argv[i], "--out") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            options->output_file = argv[i];
        } else if (strcmp(argv[i], "--json") == 0) {
            options->json_output = true;
        } else if (strcmp(argv[i], "--stats") == 0) {
            options->show_stats = true;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            criteria_cleanup(criteria);
            return -1;
        }
    }

    return 0;
}

int output_results(const search_result_t *results, size_t count, const cli_options_t *options) {
    FILE *fp = stdout;

    if (options->output_file) {
        fp = fopen(options->output_file, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open output file '%s'\n", options->output_file);
            return -1;
        }
    }

    output_format_t format = options->json_output ? OUTPUT_FORMAT_JSON : OUTPUT_FORMAT_TEXT;
    int result = output_search_results(fp, results, count, format);

    if (options->output_file) {
        fclose(fp);
    }

    return result;
}
