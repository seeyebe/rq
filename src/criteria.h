#ifndef CRITERIA_H
#define CRITERIA_H

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct search_criteria {
    char *root_path;
    char *search_term;
    char **extensions;
    size_t extensions_count;
    uint64_t min_size;
    uint64_t max_size;
    uint64_t exact_size;
    FILETIME after_time;
    FILETIME before_time;
    bool case_sensitive;
    bool use_glob;
    bool skip_common_dirs;
    bool has_min_size;
    bool has_max_size;
    bool has_exact_size;
    bool has_after_time;
    bool has_before_time;

    size_t max_threads;
    DWORD timeout_ms;
    bool follow_symlinks;
    bool include_hidden;
    size_t max_results;
    size_t max_depth;
} search_criteria_t;

void criteria_init(search_criteria_t *criteria);

bool criteria_parse_extensions(search_criteria_t *criteria, const char *extensions_str);

void criteria_cleanup(search_criteria_t *criteria);

bool criteria_validate(const search_criteria_t *criteria);

bool criteria_extension_matches(const char *filename, const search_criteria_t *criteria);

bool criteria_size_matches(uint64_t file_size, const search_criteria_t *criteria);

bool criteria_time_matches(const FILETIME *file_time, const search_criteria_t *criteria);

#endif
