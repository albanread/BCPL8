#include "heap_manager_defs.h"
#include <stdio.h>

// Global heap tracking array for all allocations
HeapBlock g_heap_blocks_array[MAX_HEAP_BLOCKS] = {};
size_t g_heap_blocks_index = 0;  // Rolling index for circular buffer

// Runtime metrics tracking
static size_t g_total_bytes_allocated = 0;
static size_t g_total_bytes_freed = 0;
static size_t g_total_allocs = 0;
static size_t g_total_frees = 0;
static size_t g_vec_allocs = 0;
static size_t g_string_allocs = 0;

// Functions to update metrics (for internal use)
void update_alloc_metrics(size_t bytes, AllocType type) {
    g_total_bytes_allocated += bytes;
    g_total_allocs++;
    
    if (type == ALLOC_VEC) {
        g_vec_allocs++;
    } else if (type == ALLOC_STRING) {
        g_string_allocs++;
    }
}

void update_free_metrics(size_t bytes) {
    g_total_bytes_freed += bytes;
    g_total_frees++;
}

// Public API: Print runtime memory metrics
void print_runtime_metrics(void) {
    printf("\n--- BCPL Runtime Metrics ---\n");
    printf("Memory allocations: %zu (%zu bytes)\n", g_total_allocs, g_total_bytes_allocated);
    printf("Memory frees: %zu (%zu bytes)\n", g_total_frees, g_total_bytes_freed);
    printf("Vector allocations: %zu\n", g_vec_allocs);
    printf("String allocations: %zu\n", g_string_allocs);
    printf("Current active allocations: %zu (%zu bytes)\n", 
           g_total_allocs - g_total_frees, 
           g_total_bytes_allocated - g_total_bytes_freed);
    printf("--------------------------\n");
}