#include <cstdlib>
#include <stdexcept>
#include <algorithm> // For std::min
#include "HeapManager.h" // Include HeapManager class definition
#include "heap_manager_defs.h" // For AllocType, HeapBlock, MAX_HEAP_BLOCKS
#include "../SignalSafeUtils.h" // For safe_print, u64_to_hex, int_to_dec

// Function is defined in SignalSafeUtils.h
extern void safe_print(const char*); // Declaration of safe_print utility

void* resizeString(void* payload, size_t newNumChars) {
    if (!payload) {
        safe_print("Error: Cannot resize a NULL string\n");
        return nullptr;
    }

    for (auto& block : g_heap_blocks_array) {
        if (block.address && static_cast<uint8_t*>(block.address) + sizeof(uint64_t) == payload) {
            if (block.type != ALLOC_STRING) {
                safe_print("Error: Attempt to resize a non-string block\n");
                return nullptr;
            }

            // Calculate new size
            size_t newTotalSize = sizeof(uint64_t) + (newNumChars + 1) * sizeof(uint32_t);

            // Resize the memory block
            void* newPtr = realloc(block.address, newTotalSize);
            if (!newPtr) {
                safe_print("Error: String resize failed\n");
                return nullptr;
            }

            // Update metadata
            block.address = newPtr;
            block.size = newTotalSize;

            // Update the length field in the string
            uint64_t* str = static_cast<uint64_t*>(newPtr);
            str[0] = newNumChars;

            // Ensure null terminator
            uint32_t* payload = reinterpret_cast<uint32_t*>(str + 1);
            payload[newNumChars] = 0;

            return static_cast<void*>(payload); // Return pointer to the payload
        }
    }

    safe_print("Error: String not found in heap tracking\n");
    return nullptr;
}