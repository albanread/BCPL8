#ifndef HEAP_MANAGER_DEFS_H
#define HEAP_MANAGER_DEFS_H

#include <cstddef> // For size_t
#include <cstdint> // For uint64_t, uint32_t

// Allocation types for the heap manager
typedef enum {
    ALLOC_UNKNOWN = 0, // Default/uninitialized
    ALLOC_VEC,         // Vector allocation (64-bit words)
    ALLOC_STRING,      // String allocation (32-bit chars)
    ALLOC_GENERIC,     // Generic allocation
    ALLOC_FREE         // Marker for freed blocks
} AllocType;

// Structure to track allocated heap blocks
// All members are basic types for signal safety
typedef struct {
    AllocType type;            // Type of allocation
    void* address;             // 64-bit pointer to the allocation
    size_t size;               // Size in bytes (64-bit length)
    const char* function_name; // Name of the function where allocation occurred
    const char* variable_name; // Name of the variable being allocated
} HeapBlock;

// Constants for heap management
#define MAX_HEAP_BLOCKS 128    // Maximum number of tracked heap blocks
#define ALIGNMENT 16           // Memory alignment for allocations

// External declarations for globals
#ifdef __cplusplus
extern "C" {
#endif

// Global heap tracking array
extern HeapBlock g_heap_blocks_array[MAX_HEAP_BLOCKS];
extern size_t g_heap_blocks_index; // Rolling index for circular buffer

// Functions to update metrics (for internal use)
void update_alloc_metrics(size_t bytes, AllocType type);
void update_free_metrics(size_t bytes);

// Metrics tracking function
void print_runtime_metrics(void);

#ifdef __cplusplus
}
#endif

#endif // HEAP_MANAGER_DEFS_H