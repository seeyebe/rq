#ifndef SEARCH_H
#define SEARCH_H

#include <windows.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct search_criteria {
    char *root_path;
    char *search_term;
    char *extensions;
    uint64_t min_size;
    uint64_t max_size;
    FILETIME after_time;
    FILETIME before_time;
    bool case_sensitive;
    bool use_glob;
    bool skip_common_dirs;
    bool has_min_size;
    bool has_max_size;
    bool has_after_time;
    bool has_before_time;
} search_criteria_t;

typedef struct search_result {
    char *path;
    uint64_t size;
    FILETIME mtime;
    struct search_result *next;
} search_result_t;

typedef struct search_context {
    search_criteria_t *criteria;
    volatile long total_results;
    search_result_t *results_head;
    search_result_t *results_tail;
    CRITICAL_SECTION results_lock;
    bool should_stop;
} search_context_t;

int search_files_fast(search_criteria_t *criteria, search_result_t **results, size_t *count);
void free_search_results(search_result_t *results);
bool matches_criteria(const WIN32_FIND_DATAA *find_data, const char *full_path,
                     const search_criteria_t *criteria);
bool matches_pattern(const char *text, const char *pattern, bool case_sensitive, bool use_glob);
bool is_system_directory(const char *path);
bool quick_extension_check(const char *filename, const char *target_ext);

#endif