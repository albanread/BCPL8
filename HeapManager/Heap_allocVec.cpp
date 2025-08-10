#include "HeapManager.h"
#include "heap_manager_defs.h"
#include "../SignalSafeUtils.h" // For safe_print, u64_to_hex, int_to_dec
#include <cstdlib>
#include <cstdint>
#include <cstddef> // For ptrdiff_t
#include <algorithm> // For std::min

void* HeapManager::allocVec(size_t numElements) {
    size_t totalSize = sizeof(uint64_t) + numElements * sizeof(uint64_t);
    void* ptr;
    if (posix_memalign(&ptr, ALIGNMENT, totalSize) != 0) {
        safe_print("Error: Vector allocation failed\n");
        return nullptr;
    }

    // Initialize vector metadata
    uint64_t* vec = static_cast<uint64_t*>(ptr);
    vec[0] = numElements; // Store length

    // Track allocation
    g_heap_blocks_array[heapIndex % MAX_HEAP_BLOCKS] = {ALLOC_VEC, ptr, totalSize, nullptr, nullptr};
    heapIndex = (heapIndex + 1) % MAX_HEAP_BLOCKS;
    // Update global index as well for metrics tracking
    g_heap_blocks_index = heapIndex;

    // Trace log
    traceLog("Allocated vector: Address=%p, Size=%zu, Elements=%zu\n", ptr, totalSize, numElements);

    // Update internal metrics
    totalBytesAllocated += totalSize;
    totalVectorsAllocated++;
    
    // Update global metrics
    update_alloc_metrics(totalSize, ALLOC_VEC);

    return static_cast<void*>(vec + 1); // Return pointer to the payload
}