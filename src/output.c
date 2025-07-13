#include "output.h"
#include "utils.h"
#include "version.h"
#include <stdio.h>
#include <inttypes.h>

static void json_escape_string(FILE *fp, const char *str) {
    if (!str) {
        fputs("null", fp);
        return;
    }

    fputc('"', fp);
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", fp); break;
            case '\\': fputc('/', fp); break;
            case '\b': fputs("\\b", fp); break;
            case '\f': fputs("\\f", fp); break;
            case '\n': fputs("\\n", fp); break;
            case '\r': fputs("\\r", fp); break;
            case '\t': fputs("\\t", fp); break;
            default:
                if ((unsigned char)*p < 0x20) {
                    fprintf(fp, "\\u%04x", (unsigned char)*p);
                } else {
                    fputc(*p, fp);
                }
                break;
        }
    }
    fputc('"', fp);
}

static void output_json_format(FILE *fp, const search_result_t *results, size_t count) {
    fputs("{\n", fp);
    fputs("  \"type\": \"search\",\n", fp);
    fprintf(fp, "  \"version\": \"%s\",\n", rq_VERSION_STRING);
    fprintf(fp, "  \"count\": %zu,\n", count);
    fputs("  \"results\": [\n", fp);

    const search_result_t *current = results;

    while (current) {
        fputs("    {\n", fp);

        fputs("      \"path\": ", fp);
        json_escape_string(fp, current->path);
        fputs(",\n", fp);

        fprintf(fp, "      \"size\": %" PRIu64 ",\n", current->size);

        char time_buffer[64];
        format_filetime_iso(&current->mtime, time_buffer, sizeof(time_buffer));
        fputs("      \"modified\": ", fp);
        json_escape_string(fp, time_buffer);
        fputs("\n", fp);

        fputs("    }", fp);
        if (current->next) {
            fputs(",", fp);
        }
        fputs("\n", fp);

        current = current->next;
    }

    fputs("  ]\n", fp);
    fputs("}\n", fp);
}

static void output_text_format(FILE *fp, const search_result_t *results, size_t count) {
    const search_result_t *current = results;

    while (current) {
        fputs(current->path, fp);
        fputc('\n', fp);
        current = current->next;
    }

    if (count > 0) {
        fprintf(stderr, "Found %zu files.\n", count);
    }
}

int output_search_results(FILE *fp, const search_result_t *results, size_t count, output_format_t format) {
    if (!fp) return -1;

    switch (format) {
        case OUTPUT_FORMAT_JSON:
            output_json_format(fp, results, count);
            break;
        case OUTPUT_FORMAT_TEXT:
        default:
            output_text_format(fp, results, count);
            break;
    }

    return 0;
}
