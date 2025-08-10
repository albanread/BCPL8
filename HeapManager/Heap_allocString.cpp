#include "HeapManager.h"
#include "heap_manager_defs.h" // For AllocType, HeapBlock, MAX_HEAP_BLOCKS
#include "../SignalSafeUtils.h" // For safe_print
#include <cstdlib>
#include <cstring>

void* HeapManager::allocString(size_t numChars) {
    size_t totalSize = sizeof(uint64_t) + (numChars + 1) * sizeof(uint32_t);
    void* ptr;
    if (posix_memalign(&ptr, ALIGNMENT, totalSize) != 0) {
        safe_print("Error: String allocation failed\n");
        return nullptr;
    }

    // Initialize string metadata
    uint64_t* str = static_cast<uint64_t*>(ptr);
    str[0] = numChars; // Store length
    uint32_t* payload = reinterpret_cast<uint32_t*>(str + 1);
    payload[numChars] = 0; // Null terminator

    // Track allocation
    g_heap_blocks_array[heapIndex % MAX_HEAP_BLOCKS] = {ALLOC_STRING, ptr, totalSize, nullptr, nullptr};
    heapIndex = (heapIndex + 1) % MAX_HEAP_BLOCKS; // Ensure circular buffer logic
    g_heap_blocks_index = heapIndex; // Update global index as well for metrics tracking

    // Trace log
    traceLog("Allocated string: Address=%p, Size=%zu, Characters=%zu\n", ptr, totalSize, numChars);

    // Update internal metrics
    totalBytesAllocated += totalSize;
    totalStringsAllocated++;
    
    // Update global metrics
    update_alloc_metrics(totalSize, ALLOC_STRING);

    return static_cast<void*>(payload); // Return pointer to the payload
}