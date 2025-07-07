#include "search.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <process.h>

#define MAX_THREADS 16
#define THREAD_STACK_SIZE 65536
#define SEARCH_TIMEOUT_MS 300000
#define MAX_PATH_SAFE 4096

typedef struct {
    HANDLE semaphore;
    HANDLE threads[MAX_THREADS];
    volatile LONG active_count;
    volatile LONG total_created;
} thread_pool_t;

typedef struct {
    search_context_t *ctx;
    char directory[MAX_PATH_SAFE];
    char extensions_copy[256];
} thread_data_t;

static thread_pool_t g_thread_pool = {0};

static bool safe_strcpy(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) return false;

    size_t src_len = strlen(src);
    if (src_len >= dest_size) return false;

    memcpy(dest, src, src_len + 1);
    return true;
}

static bool safe_strcat(char *dest, size_t dest_size, const char *src) {
    if (!dest || !src || dest_size == 0) return false;

    size_t dest_len = strlen(dest);
    size_t src_len = strlen(src);

    if (dest_len + src_len >= dest_size) return false;

    memcpy(dest + dest_len, src, src_len + 1);
    return true;
}

static char* safe_strdup(const char *str) {
    if (!str) return NULL;

    size_t len = strlen(str);
    char *copy = malloc(len + 1);
    if (!copy) return NULL;

    memcpy(copy, str, len + 1);
    return copy;
}

static void safe_strlwr(char *str) {
    if (!str) return;
    while (*str) {
        *str = tolower(*str);
        str++;
    }
}

static bool init_thread_pool(void) {
    g_thread_pool.semaphore = CreateSemaphore(NULL, MAX_THREADS, MAX_THREADS, NULL);
    if (!g_thread_pool.semaphore) return false;

    g_thread_pool.active_count = 0;
    g_thread_pool.total_created = 0;

    for (int i = 0; i < MAX_THREADS; i++) {
        g_thread_pool.threads[i] = NULL;
    }

    return true;
}

static void cleanup_thread_pool(void) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_thread_pool.threads[i]) {
            WaitForSingleObject(g_thread_pool.threads[i], INFINITE);
            CloseHandle(g_thread_pool.threads[i]);
            g_thread_pool.threads[i] = NULL;
        }
    }

    if (g_thread_pool.semaphore) {
        CloseHandle(g_thread_pool.semaphore);
        g_thread_pool.semaphore = NULL;
    }
}

bool matches_pattern(const char *text, const char *pattern, bool case_sensitive, bool use_glob) {
    if (!text || !pattern) return false;

    if (pattern[0] == '\0' || (pattern[0] == '*' && pattern[1] == '\0'))
        return true;

    if (use_glob && (strchr(pattern, '*') || strchr(pattern, '?'))) {
        const char *t = text;
        const char *p = pattern;

        while (*t && *p) {
            if (*p == '*') {
                p++;
                if (!*p) return true;

                while (*t) {
                    if (matches_pattern(t, p, case_sensitive, use_glob)) {
                        return true;
                    }
                    t++;
                }
                return false;
            } else if (*p == '?') {
                t++;
                p++;
            } else {
                char tc = case_sensitive ? *t : tolower(*t);
                char pc = case_sensitive ? *p : tolower(*p);

                if (tc != pc) return false;
                t++;
                p++;
            }
        }

        while (*p == '*') p++;
        return !*t && !*p;
    } else {
        if (case_sensitive) {
            return strstr(text, pattern) != NULL;
        } else {
            char *text_lower = safe_strdup(text);
            char *pattern_lower = safe_strdup(pattern);

            if (!text_lower || !pattern_lower) {
                free(text_lower);
                free(pattern_lower);
                return false;
            }

            safe_strlwr(text_lower);
            safe_strlwr(pattern_lower);

            bool result = strstr(text_lower, pattern_lower) != NULL;

            free(text_lower);
            free(pattern_lower);

            return result;
        }
    }
}

bool matches_criteria(const WIN32_FIND_DATAA *find_data, const char *full_path,
                     const search_criteria_t *criteria) {
    if (!find_data || !criteria) return false;

    // Skip directories
    if (find_data->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        return false;
    }

    // Check filename pattern
    if (!matches_pattern(find_data->cFileName, criteria->search_term,
                        criteria->case_sensitive, criteria->use_glob))
        return false;

    // Check file size
    uint64_t file_size = ((uint64_t)find_data->nFileSizeHigh << 32) | find_data->nFileSizeLow;
    if (criteria->has_min_size && file_size < criteria->min_size) {
        return false;
    }
    if (criteria->has_max_size && file_size > criteria->max_size) {
        return false;
    }

    // Check modification time
    if (criteria->has_after_time) {
        if (CompareFileTime(&find_data->ftLastWriteTime, &criteria->after_time) < 0) {
            return false;
        }
    }
    if (criteria->has_before_time) {
        if (CompareFileTime(&find_data->ftLastWriteTime, &criteria->before_time) > 0) {
            return false;
        }
    }

    if (criteria->extensions && strlen(criteria->extensions) > 0) {
        const char *ext = strrchr(find_data->cFileName, '.');
        if (!ext) return false;

        char *ext_copy = safe_strdup(criteria->extensions);
        if (!ext_copy) return false;

        bool found = false;
        char *token = strtok(ext_copy, ",");

        while (token && !found) {
            while (isspace(*token)) token++;

            if (*token == '.') token++;

            if (_stricmp(ext + 1, token) == 0) {
                found = true;
            }
            token = strtok(NULL, ",");
        }

        free(ext_copy);
        return found;
    }

    return true;
}

void free_search_results(search_result_t *results) {
    while (results) {
        search_result_t *next = results->next;
        free(results->path);
        free(results);
        results = next;
    }
}

static const char* system_paths[] = {
    "\\$Recycle.Bin", "\\System Volume Information", "\\Windows\\System32",
    "\\Windows\\SysWOW64", "\\Program Files", "\\Program Files (x86)",
    "\\ProgramData", "\\Recovery", "\\Intel", "\\AMD", "\\NVIDIA",
    "\\hiberfil.sys", "\\pagefile.sys", "\\swapfile.sys"
};

static const char* skip_directories[] = {
    "$RECYCLE.BIN", "System Volume Information", "Windows", "Program Files",
    "Program Files (x86)", "ProgramData", "Recovery", "Intel", "AMD", "NVIDIA",
    "node_modules", ".git", ".svn", "__pycache__", "obj", "bin", "Debug",
    "Release", ".vs", "packages", "bower_components", "dist", "build"
};

bool is_system_directory(const char *path) {
    if (!path) return true;

    for (size_t i = 0; i < sizeof(system_paths) / sizeof(system_paths[0]); i++) {
        if (strstr(path, system_paths[i]) != NULL) {
            return true;
        }
    }
    return false;
}

bool should_skip_directory(const char *dirname) {
    if (!dirname) return true;

    for (size_t i = 0; i < sizeof(skip_directories) / sizeof(skip_directories[0]); i++) {
        if (_stricmp(dirname, skip_directories[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool quick_extension_check(const char *filename, const char *target_ext) {
    if (!target_ext || !filename || strlen(target_ext) == 0) return true;

    const char *dot = strrchr(filename, '.');
    if (!dot) return false;

    return _stricmp(dot, target_ext) == 0;
}

static bool add_result_safe(search_context_t *ctx, const char *path,
                           uint64_t size, FILETIME mtime) {
    if (!ctx || !path) return false;

    search_result_t *result = malloc(sizeof(search_result_t));
    if (!result) return false;

    result->path = safe_strdup(path);
    if (!result->path) {
        free(result);
        return false;
    }

    result->size = size;
    result->mtime = mtime;
    result->next = NULL;

    EnterCriticalSection(&ctx->results_lock);

    if (!ctx->results_head) {
        ctx->results_head = result;
        ctx->results_tail = result;
    } else {
        ctx->results_tail->next = result;
        ctx->results_tail = result;
    }

    InterlockedIncrement(&ctx->total_results);

    LeaveCriticalSection(&ctx->results_lock);
    return true;
}

static void process_directory_safe(search_context_t *ctx, const char *directory,
                                  const char *extensions_copy);

static unsigned __stdcall process_directory_thread(void *param) {
    thread_data_t *data = (thread_data_t*)param;

    process_directory_safe(data->ctx, data->directory, data->extensions_copy);

    for (int i = 0; i < MAX_THREADS; i++) {
        if (g_thread_pool.threads[i] == (HANDLE)_beginthreadex(NULL, 0, NULL, NULL, CREATE_SUSPENDED, NULL)) {
            CloseHandle(g_thread_pool.threads[i]);
            g_thread_pool.threads[i] = NULL;
            break;
        }
    }

    InterlockedDecrement(&g_thread_pool.active_count);
    ReleaseSemaphore(g_thread_pool.semaphore, 1, NULL);

    free(data);
    return 0;
}

static bool spawn_directory_thread(search_context_t *ctx, const char *directory,
                                  const char *extensions_copy) {
    // Wait for thread slot
    if (WaitForSingleObject(g_thread_pool.semaphore, 1000) != WAIT_OBJECT_0) {
        return false;
    }

    thread_data_t *data = malloc(sizeof(thread_data_t));
    if (!data) {
        ReleaseSemaphore(g_thread_pool.semaphore, 1, NULL);
        return false;
    }

    data->ctx = ctx;
    if (!safe_strcpy(data->directory, sizeof(data->directory), directory) ||
        !safe_strcpy(data->extensions_copy, sizeof(data->extensions_copy),
                    extensions_copy ? extensions_copy : "")) {
        free(data);
        ReleaseSemaphore(g_thread_pool.semaphore, 1, NULL);
        return false;
    }

    int slot = -1;
    for (int i = 0; i < MAX_THREADS; i++) {
        if (!g_thread_pool.threads[i]) {
            slot = i;
            break;
        }
    }

    if (slot == -1) {
        free(data);
        ReleaseSemaphore(g_thread_pool.semaphore, 1, NULL);
        process_directory_safe(ctx, directory, extensions_copy);
        return true;
    }

    unsigned thread_id;
    HANDLE thread = (HANDLE)_beginthreadex(NULL, THREAD_STACK_SIZE,
                                          process_directory_thread, data, 0, &thread_id);

    if (!thread) {
        free(data);
        ReleaseSemaphore(g_thread_pool.semaphore, 1, NULL);
        return false;
    }

    g_thread_pool.threads[slot] = thread;
    InterlockedIncrement(&g_thread_pool.active_count);
    InterlockedIncrement(&g_thread_pool.total_created);

    return true;
}

static void process_directory_safe(search_context_t *ctx, const char *directory,
                                  const char *extensions_copy) {
    if (!ctx || !directory || ctx->should_stop || is_system_directory(directory)) {
        return;
    }

    char search_pattern[MAX_PATH_SAFE];
    if (!safe_strcpy(search_pattern, sizeof(search_pattern), directory) ||
        !safe_strcat(search_pattern, sizeof(search_pattern), "\\*")) {
        return;
    }

    WIN32_FIND_DATAA find_data;
    HANDLE find_handle = FindFirstFileA(search_pattern, &find_data);

    if (find_handle == INVALID_HANDLE_VALUE) {
        return;
    }

    char target_ext[32] = {0};
    if (extensions_copy && strlen(extensions_copy) > 0) {
        char *ext_copy = safe_strdup(extensions_copy);
        if (ext_copy) {
            char *first_ext = strtok(ext_copy, ",");
            if (first_ext) {
                // Trim whitespace
                while (isspace(*first_ext)) first_ext++;

                if (*first_ext == '.') {
                    safe_strcpy(target_ext, sizeof(target_ext), first_ext);
                } else {
                    target_ext[0] = '.';
                    safe_strcpy(target_ext + 1, sizeof(target_ext) - 1, first_ext);
                }
            }
            free(ext_copy);
        }
    }

    do {
        if (ctx->should_stop) break;

        if (strcmp(find_data.cFileName, ".") == 0 ||
            strcmp(find_data.cFileName, "..") == 0) {
            continue;
        }

        char full_path[MAX_PATH_SAFE];
        if (!safe_strcpy(full_path, sizeof(full_path), directory) ||
            !safe_strcat(full_path, sizeof(full_path), "\\") ||
            !safe_strcat(full_path, sizeof(full_path), find_data.cFileName)) {
            continue;
        }

        if (find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
            if (!ctx->criteria->skip_common_dirs || !should_skip_directory(find_data.cFileName)) {
                spawn_directory_thread(ctx, full_path, extensions_copy);
            }
        } else {
            if (target_ext[0] && !quick_extension_check(find_data.cFileName, target_ext)) {
                continue;
            }

            if (matches_criteria(&find_data, full_path, ctx->criteria)) {
                uint64_t file_size = ((uint64_t)find_data.nFileSizeHigh << 32) |
                                   find_data.nFileSizeLow;
                add_result_safe(ctx, full_path, file_size, find_data.ftLastWriteTime);
            }
        }

    } while (FindNextFileA(find_handle, &find_data) && !ctx->should_stop);

    FindClose(find_handle);
}

int search_files_fast(search_criteria_t *criteria, search_result_t **results, size_t *count) {
    if (!criteria || !results || !count) {
        return -1;
    }

    *results = NULL;
    *count = 0;

    if (!init_thread_pool()) {
        return -1;
    }

    search_context_t ctx = {0};
    ctx.criteria = criteria;
    ctx.should_stop = false;
    ctx.total_results = 0;
    ctx.results_head = NULL;
    ctx.results_tail = NULL;

    if (!InitializeCriticalSectionAndSpinCount(&ctx.results_lock, 4000)) {
        cleanup_thread_pool();
        return -1;
    }

    DWORD start_time = GetTickCount();

    char *extensions_copy = NULL;
    if (criteria->extensions) {
        extensions_copy = safe_strdup(criteria->extensions);
    }

    process_directory_safe(&ctx, criteria->root_path, extensions_copy);

    while (g_thread_pool.active_count > 0 && !ctx.should_stop) {
        Sleep(100);

        DWORD elapsed = GetTickCount() - start_time;
        if (elapsed > SEARCH_TIMEOUT_MS) {
            printf("Search timeout reached (%d ms), stopping...\n", SEARCH_TIMEOUT_MS);
            ctx.should_stop = true;
            break;
        }

        if ((elapsed % 5000) < 100) {
            printf("Search in progress... Found %ld files, %ld threads active\n",
                   ctx.total_results, g_thread_pool.active_count);
        }
    }

    DWORD elapsed = GetTickCount() - start_time;
    printf("Search completed in %lu ms\n", elapsed);
    printf("Found %ld files using %ld threads\n", ctx.total_results, g_thread_pool.total_created);

    cleanup_thread_pool();
    free(extensions_copy);

    *results = ctx.results_head;
    *count = ctx.total_results;

    DeleteCriticalSection(&ctx.results_lock);
    return 0;
}