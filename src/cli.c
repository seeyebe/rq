#include "cli.h"
#include "criteria.h"
#include "utils.h"
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
    printf("Usage: %s <root_directory> <search_term> [options]\n\n", program_name);
    printf("Search Options:\n");
    printf("  --case              Case-sensitive search\n");
    printf("  --glob              Enable glob patterns (* ? [] {})\n");
    printf("  --no-skip           Don't skip common directories (node_modules, .git, etc.)\n");
    printf("  --ext <extensions>  Filter by file extensions (comma-separated)\n");
    printf("  --min <size>        Minimum file size (supports K, M, G, T suffixes)\n");
    printf("  --max <size>        Maximum file size (supports K, M, G, T suffixes)\n");
    printf("  --after <date>      Files modified after date (YYYY-MM-DD)\n");
    printf("  --before <date>     Files modified before date (YYYY-MM-DD)\n");
    printf("  --max-results <n>   Maximum number of results (0 = unlimited)\n");
    printf("  --max-depth <n>     Maximum recursion depth (0 = unlimited)\n");
    printf("  --follow-symlinks   Follow symbolic links\n");
    printf("  --include-hidden    Include hidden files and directories\n\n");
    printf("Performance Options:\n");
    printf("  --threads <n>       Number of worker threads (0 = auto)\n");
    printf("  --timeout <ms>      Search timeout in milliseconds\n\n");
    printf("Output Options:\n");
    printf("  --out <file>        Write output to file\n");
    printf("  --json              Output results as JSON\n");
    printf("  --help              Show this help message\n");
    printf("  --version           Show version information\n\n");
    printf("Glob Patterns:\n");
    printf("  *                   Matches any sequence of characters\n");
    printf("  ?                   Matches any single character\n");
    printf("  [abc]               Matches any character in the set\n");
    printf("  [a-z]               Matches any character in the range\n");
    printf("  [!abc]              Matches any character NOT in the set\n");
    printf("  {jpg,png,gif}       Matches any of the specified alternatives\n");
    printf("  \\*                  Escapes special characters\n\n");
    printf("Examples:\n");
    printf("  %s D:\\ \"*.png\" --ext png,jpg\n", program_name);
    printf("  %s . document --ext pdf,docx --min 1K --max 10M\n", program_name);
    printf("  %s C:\\ \"project_[0-9]*\" --after 2024-01-01 --json\n", program_name);
    printf("  %s /home \"*.{cpp,h}\" --threads 8 --max-results 1000\n", program_name);
}

void print_version(void) {
    printf("snub v1.0.0\n");
    printf("Copyright (c) 2025. Open source under MIT license.\n");
}

int parse_command_line(int argc, char *argv[], search_criteria_t *criteria, cli_options_t *options) {
    if (argc < 2) {
        return -1;
    }

    criteria_init(criteria);
    init_options(options);

    if (strcmp(argv[1], "--help") == 0) {
        options->show_help = true;
        return 0;
    }

    if (strcmp(argv[1], "--version") == 0) {
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
        if (strcmp(argv[i], "--case") == 0) {
            criteria->case_sensitive = true;
        } else if (strcmp(argv[i], "--glob") == 0) {
            criteria->use_glob = true;
        } else if (strcmp(argv[i], "--no-skip") == 0) {
            criteria->skip_common_dirs = false;
        } else if (strcmp(argv[i], "--follow-symlinks") == 0) {
            criteria->follow_symlinks = true;
        } else if (strcmp(argv[i], "--include-hidden") == 0) {
            criteria->include_hidden = true;
        } else if (strcmp(argv[i], "--ext") == 0) {
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
        } else if (strcmp(argv[i], "--max-depth") == 0) {
            if (++i >= argc) {
                criteria_cleanup(criteria);
                return -1;
            }
            criteria->max_depth = (size_t)strtoull(argv[i], NULL, 10);
        } else if (strcmp(argv[i], "--threads") == 0) {
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
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            criteria_cleanup(criteria);
            return -1;
        }
    }

    return 0;
}

static void output_json_results(FILE *fp, const search_result_t *results, size_t count) {
    fprintf(fp, "{\n");
    fprintf(fp, "  \"count\": %zu,\n", count);
    fprintf(fp, "  \"results\": [\n");

    bool first = true;
    for (const search_result_t *result = results; result; result = result->next) {
        if (!first) {
            fprintf(fp, ",\n");
        }
        first = false;

        char time_str[32];
        format_filetime_iso(&result->mtime, time_str, sizeof(time_str));

        fprintf(fp, "    {\n");
        fprintf(fp, "      \"path\": \"%s\",\n", result->path);
        fprintf(fp, "      \"size\": %llu,\n", result->size);
        fprintf(fp, "      \"mtime\": \"%s\"\n", time_str);
        fprintf(fp, "    }");
    }

    fprintf(fp, "\n  ]\n}\n");
}

static void output_text_results(FILE *fp, const search_result_t *results, size_t count) {
    for (const search_result_t *result = results; result; result = result->next) {
        fprintf(fp, "%s\n", result->path);
    }

    if (count > 0) {
        fprintf(stderr, "\nFound %zu files.\n", count);
    }
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

    if (options->json_output) {
        output_json_results(fp, results, count);
    } else {
        output_text_results(fp, results, count);
    }

    if (options->output_file) {
        fclose(fp);
    }

    return 0;
}
