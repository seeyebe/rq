#include "thread_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

struct thread_pool {
    PTP_POOL pool;
    TP_CALLBACK_ENVIRON callback_environ;
    PTP_CLEANUP_GROUP cleanup_group;
    atomic_size_t active_work_items;
    atomic_size_t completed_work_items;
    atomic_size_t total_submitted;
    atomic_size_t queued_work_items;  // Track queued items ourselves
    thread_pool_config_t config;
    CRITICAL_SECTION stats_lock;
};

typedef struct {
    work_function_t work_func;
    void *user_data;
    thread_pool_t *pool;
} work_item_t;

static VOID CALLBACK thread_pool_work_callback(PTP_CALLBACK_INSTANCE instance, PVOID context, PTP_WORK work) {
    (void)instance;

    work_item_t *item = (work_item_t*)context;

    atomic_fetch_sub(&item->pool->queued_work_items, 1);

    if (item->pool->config.stop_flag && atomic_load(item->pool->config.stop_flag)) {
        goto cleanup;
    }

    item->work_func(item, item->user_data);

    atomic_fetch_add(&item->pool->completed_work_items, 1);
    atomic_fetch_sub(&item->pool->active_work_items, 1);

cleanup:
    free(item);
    CloseThreadpoolWork(work);
}

thread_pool_t* thread_pool_create(const thread_pool_config_t *config) {
    if (!config) return NULL;

    thread_pool_t *pool = calloc(1, sizeof(thread_pool_t));
    if (!pool) return NULL;

    pool->config = *config;

    atomic_init(&pool->active_work_items, 0);
    atomic_init(&pool->completed_work_items, 0);
    atomic_init(&pool->total_submitted, 0);
    atomic_init(&pool->queued_work_items, 0);

    if (!InitializeCriticalSectionAndSpinCount(&pool->stats_lock, 4000)) {
        free(pool);
        return NULL;
    }

    pool->pool = CreateThreadpool(NULL);
    if (!pool->pool) {
        DeleteCriticalSection(&pool->stats_lock);
        free(pool);
        return NULL;
    }

    if (config->max_threads > 0) {
        SetThreadpoolThreadMaximum(pool->pool, (DWORD)config->max_threads);
        SetThreadpoolThreadMinimum(pool->pool, 1);
    }

    InitializeThreadpoolEnvironment(&pool->callback_environ);
    SetThreadpoolCallbackPool(&pool->callback_environ, pool->pool);

    pool->cleanup_group = CreateThreadpoolCleanupGroup();
    if (pool->cleanup_group) {
        SetThreadpoolCallbackCleanupGroup(&pool->callback_environ, pool->cleanup_group, NULL);
    }

    return pool;
}

bool thread_pool_submit(thread_pool_t *pool, work_function_t work_func, void *user_data) {
    if (!pool || !work_func) return false;

    if (pool->config.stop_flag && atomic_load(pool->config.stop_flag)) {
        return false;
    }

    work_item_t *item = malloc(sizeof(work_item_t));
    if (!item) return false;

    item->work_func = work_func;
    item->user_data = user_data;
    item->pool = pool;

    PTP_WORK work = CreateThreadpoolWork(thread_pool_work_callback, item, &pool->callback_environ);
    if (!work) {
        free(item);
        return false;
    }

    atomic_fetch_add(&pool->queued_work_items, 1);
    atomic_fetch_add(&pool->total_submitted, 1);

    SubmitThreadpoolWork(work);

    atomic_fetch_add(&pool->active_work_items, 1);

    return true;
}

bool thread_pool_wait_completion(thread_pool_t *pool, DWORD timeout_ms) {
    if (!pool) return false;

    DWORD start_time = GetTickCount();

    while (atomic_load(&pool->active_work_items) > 0) {
        if (timeout_ms != INFINITE) {
            DWORD elapsed = GetTickCount() - start_time;
            if (elapsed >= timeout_ms) {
                return false;
            }
        }

        if (pool->config.stop_flag && atomic_load(pool->config.stop_flag)) {
            break;
        }

        if (pool->config.progress_cb) {
            size_t completed = atomic_load(&pool->completed_work_items);
            size_t active = atomic_load(&pool->active_work_items);

            if (!pool->config.progress_cb(completed, active, pool->config.progress_user_data)) {
                if (pool->config.stop_flag) {
                    atomic_store(pool->config.stop_flag, true);
                }
                break;
            }
        }

        Sleep(10);
    }

    return true;
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (!pool) return;

    if (pool->config.stop_flag) {
        atomic_store(pool->config.stop_flag, true);
    }

    if (pool->cleanup_group) {
        CloseThreadpoolCleanupGroupMembers(pool->cleanup_group, FALSE, NULL);
        CloseThreadpoolCleanupGroup(pool->cleanup_group);
    }

    DestroyThreadpoolEnvironment(&pool->callback_environ);

    if (pool->pool) {
        CloseThreadpool(pool->pool);
    }

    DeleteCriticalSection(&pool->stats_lock);
    free(pool);
}

bool thread_pool_get_stats(thread_pool_t *pool, thread_pool_stats_t *stats) {
    if (!pool || !stats) return false;

    EnterCriticalSection(&pool->stats_lock);

    stats->active_threads = atomic_load(&pool->active_work_items);
    stats->queued_work_items = atomic_load(&pool->queued_work_items);
    stats->completed_work_items = atomic_load(&pool->completed_work_items);
    stats->total_submitted = atomic_load(&pool->total_submitted);

    LeaveCriticalSection(&pool->stats_lock);
    return true;
}