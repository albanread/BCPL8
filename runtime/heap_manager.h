#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// For C++ compatibility
#ifdef __cplusplus
extern "C" {
#endif

// --- C-compatible definitions ---

typedef enum {
    ALLOC_FREE = 0,
    ALLOC_VEC,
    ALLOC_STRING
} AllocType;

typedef struct {
    AllocType type;
    void* address;
    size_t size;
} HeapBlock;

// --- Public API ---

// Initialization (if needed)
void heap_init(void);

// Allocation and Deallocation
void* heap_alloc_vec(size_t num_elements);
void* heap_alloc_string(size_t num_chars);
void heap_free(void* payload);

// Resizing
void* heap_resize_vec(void* payload, size_t new_num_elements);
void* heap_resize_string(void* payload, size_t new_num_chars);

// Diagnostics and Tracing
void heap_set_trace(bool enable);
void heap_print_metrics(void);
void heap_dump(void);

#ifdef __cplusplus
}
#endif

#endif // HEAP_MANAGER_H