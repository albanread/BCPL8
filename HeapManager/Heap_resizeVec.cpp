#include "HeapManager.h"
#include "heap_manager_defs.h" // For AllocType, HeapBlock, MAX_HEAP_BLOCKS
#include "../SignalSafeUtils.h" // For safe_print, int_to_dec
#include <cstdlib>
#include <stdexcept>
#include <algorithm> // For std::min
#include <cstdint>

// Resize a vector
void* resizeVec(void* payload, size_t newNumElements) {
    if (!payload) {
        safe_print("Error: Cannot resize a NULL vector\n");
        return nullptr;
    }

    for (auto& block : g_heap_blocks_array) {
        if (block.address && static_cast<uint8_t*>(block.address) + sizeof(uint64_t) == payload) {
            if (block.type != ALLOC_VEC) {
                safe_print("Error: Attempt to resize a non-vector block\n");
                return nullptr;
            }

            // Calculate new size
            size_t newTotalSize = sizeof(uint64_t) + newNumElements * sizeof(uint64_t);

            // Resize the memory block
            void* newPtr = realloc(block.address, newTotalSize);
            if (!newPtr) {
                safe_print("Error: Vector resize failed\n");
                return nullptr;
            }

            // Update metadata
            block.address = newPtr;
            block.size = newTotalSize;

            // Update the length field in the vector
            uint64_t* vec = static_cast<uint64_t*>(newPtr);
            vec[0] = newNumElements;

            return static_cast<void*>(vec + 1); // Return pointer to the payload
        }
    }

    safe_print("Error: Vector not found in heap tracking\n");
    return nullptr;
}
