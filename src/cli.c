#include "cli.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void init_criteria(search_criteria_t *criteria) {
    memset(criteria, 0, sizeof(search_criteria_t));
    criteria->case_sensitive = false;
    criteria->use_glob = true;
    criteria->skip_common_dirs = true;
}

static void init_options(cli_options_t *options) {
    memset(options, 0, sizeof(cli_options_t));
}

static int parse_size_arg(const char *arg, uint64_t *size) {
    char *endptr;
    *size = strtoull(arg, &endptr, 10);
    if (*endptr != '\0' || *size == 0) {
        return -1;
    }
    return 0;
}

static int parse_date_arg(const char *arg, FILETIME *file_time) {
    return parse_date_string(arg, file_time);
}

void print_usage(const char *program_name) {
    printf("Usage: %s <root_directory> <search_term> [options]\n\n", program_name);
    printf("Options:\n");
    printf("  --case              Case-sensitive search\n");
    printf("  --glob              Enable glob patterns (* and ?)\n");
    printf("  --no-skip           Don't skip common directories (node_modules, .git, etc.)\n");
    printf("  --ext <extensions>  Filter by file extensions (comma-separated)\n");
    printf("  --min <size>        Minimum file size in bytes\n");
    printf("  --max <size>        Maximum file size in bytes\n");
    printf("  --after <date>      Files modified after date (YYYY-MM-DD)\n");
    printf("  --before <date>     Files modified before date (YYYY-MM-DD)\n");
    printf("  --out <file>        Write output to file\n");
    printf("  --json              Output results as JSON\n");
    printf("  --help              Show this help message\n");
    printf("  --version           Show version information\n\n");
    printf("Examples:\n");
    printf("  %s D:\\ *.png --ext .png,.jpg\n", program_name);
    printf("  %s . document --ext .pdf,.docx --min 1024\n", program_name);
    printf("  %s C:\\ project --after 2024-01-01 --json\n", program_name);
}

void print_version(void) {
    printf("snub v0.1.0\n");
    printf("Fast recursive file search utility for Windows\n");
}

int parse_command_line(int argc, char *argv[], search_criteria_t *criteria, cli_options_t *options) {
    if (argc < 2) {
        return -1;
    }

    init_criteria(criteria);
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

    criteria->root_path = argv[1];
    criteria->search_term = argv[2];

    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "--case") == 0) {
            criteria->case_sensitive = true;
        } else if (strcmp(argv[i], "--glob") == 0) {
            criteria->use_glob = true;
        } else if (strcmp(argv[i], "--no-skip") == 0) {
            criteria->skip_common_dirs = false;
        } else if (strcmp(argv[i], "--ext") == 0) {
            if (++i >= argc) return -1;
            criteria->extensions = argv[i];
        } else if (strcmp(argv[i], "--min") == 0) {
            if (++i >= argc) return -1;
            if (parse_size_arg(argv[i], &criteria->min_size) != 0) return -1;
            criteria->has_min_size = true;
        } else if (strcmp(argv[i], "--max") == 0) {
            if (++i >= argc) return -1;
            if (parse_size_arg(argv[i], &criteria->max_size) != 0) return -1;
            criteria->has_max_size = true;
        } else if (strcmp(argv[i], "--after") == 0) {
            if (++i >= argc) return -1;
            if (parse_date_arg(argv[i], &criteria->after_time) != 0) return -1;
            criteria->has_after_time = true;
        } else if (strcmp(argv[i], "--before") == 0) {
            if (++i >= argc) return -1;
            if (parse_date_arg(argv[i], &criteria->before_time) != 0) return -1;
            criteria->has_before_time = true;
        } else if (strcmp(argv[i], "--out") == 0) {
            if (++i >= argc) return -1;
            options->output_file = argv[i];
        } else if (strcmp(argv[i], "--json") == 0) {
            options->json_output = true;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return -1;
        }
    }

    return 0;
}

static void output_json_results(FILE *fp, const search_result_t *results) {
    fprintf(fp, "[\n");
    bool first = true;

    for (const search_result_t *result = results; result; result = result->next) {
        if (!first) {
            fprintf(fp, ",\n");
        }
        first = false;

        char time_str[32];
        format_filetime_iso(&result->mtime, time_str, sizeof(time_str));

        fprintf(fp, "  {\n");
        fprintf(fp, "    \"path\": \"%s\",\n", result->path);
        fprintf(fp, "    \"size\": %llu,\n", result->size);
        fprintf(fp, "    \"mtime\": \"%s\"\n", time_str);
        fprintf(fp, "  }");
    }

    fprintf(fp, "\n]\n");
}

static void output_text_results(FILE *fp, const search_result_t *results) {
    for (const search_result_t *result = results; result; result = result->next) {
        fprintf(fp, "%s\n", result->path);
    }
}

int output_results(const search_result_t *results, size_t count, const cli_options_t *options) {
    FILE *fp = stdout;
    (void)count;

    if (options->output_file) {
        fp = fopen(options->output_file, "w");
        if (!fp) {
            fprintf(stderr, "Error: Cannot open output file '%s'\n", options->output_file);
            return -1;
        }
    }

    if (options->json_output) {
        output_json_results(fp, results);
    } else {
        output_text_results(fp, results);
    }

    if (options->output_file) {
        fclose(fp);
    }

    return 0;
}
