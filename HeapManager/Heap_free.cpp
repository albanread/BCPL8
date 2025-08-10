#include <cstdlib>
#include "HeapManager.h" 
#include <cstddef> // For size_t
#include "heap_manager_defs.h" // For AllocType, HeapBlock, MAX_HEAP_BLOCKS
#include "../SignalSafeUtils.h" // For safe_print

void HeapManager::free(void* payload) {
    if (!payload) return;

    for (size_t i = 0; i < MAX_HEAP_BLOCKS; ++i) {
        auto& block = g_heap_blocks_array[i]; // Use a reference to modify

        // Check if this block corresponds to the payload.
        if (block.address && (static_cast<uint8_t*>(block.address) + sizeof(uint64_t) == payload)) {
            std::free(block.address);

            // Update internal metrics
            totalBytesFreed += block.size;
            if (block.type == ALLOC_VEC) {
                totalVectorsFreed++;
            } else if (block.type == ALLOC_STRING) {
                totalStringsFreed++;
            }
            
            // Update global metrics
            update_free_metrics(block.size);

            // Reset block information
            block.type = ALLOC_FREE; // Mark the block as free
            block.size = 0;
            block.address = nullptr;

            // Trace log if enabled
            traceLog("Freed memory: Address=%p\n", payload);

            return; // Found and freed, exit loop
        }
    }

    // If loop completes without finding the block
    safe_print("Error: Attempt to free untracked memory\n");
}