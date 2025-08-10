#include "HeapManager.h"
#include "heap_manager_defs.h" // For AllocType, HeapBlock, MAX_HEAP_BLOCKS
#include "../SignalSafeUtils.h" // For safe_print, u64_to_hex, int_to_dec
#include <algorithm> // For std::min

void HeapManager::dumpHeap() const {
    safe_print("\n=== Debug: Heap Blocks ===\n");
    char addr_buf[20], size_buf[20], type_buf[20];
    for (size_t i = 0; i < MAX_HEAP_BLOCKS; ++i) {
        const auto& block = g_heap_blocks_array[i];
        if (block.type != ALLOC_FREE && block.address != nullptr) {
            safe_print("Block ");
            int_to_dec((int)i, type_buf);
            safe_print(type_buf);
            safe_print(": Type=");
            int_to_dec((int)block.type, type_buf);
            safe_print(type_buf);
            safe_print(", Address=");
            u64_to_hex((uint64_t)(uintptr_t)block.address, addr_buf);
            safe_print(addr_buf);
            safe_print(", Size=");
            int_to_dec((int64_t)block.size, size_buf);
            safe_print(size_buf);
            safe_print("\n");
        }
    }

    safe_print("\n=== Heap Dump ===\n");
    char elem_buf[20];
    size_t count = 0;
    for (size_t i = 0; i < MAX_HEAP_BLOCKS; ++i) {
        const auto& block = g_heap_blocks_array[i];
        if (block.type != ALLOC_FREE) {
            safe_print("Block ");
            int_to_dec((int)i, type_buf);
            safe_print(type_buf);
            safe_print(": Type=");
            int_to_dec((int)block.type, type_buf);
            safe_print(type_buf);
            safe_print(", Size=");
            int_to_dec((int64_t)block.size, size_buf);
            safe_print(size_buf);
            safe_print(", Address=");
            u64_to_hex((uint64_t)(uintptr_t)block.address, addr_buf);
            safe_print(addr_buf);
            safe_print("\n");
            if (block.type == ALLOC_VEC) {
                safe_print("  Vector length: ");
                uint64_t* vec = static_cast<uint64_t*>(block.address);
                size_t len = vec[0];
                char len_buf[20];
                int_to_dec((int64_t)len, len_buf);
                safe_print(len_buf);
                safe_print("\n  Elements: ");
                for (size_t j = 0; j < std::min(len, size_t(10)); ++j) {
                    int_to_dec((int64_t)vec[1 + j], elem_buf);
                    safe_print(elem_buf);
                    safe_print(" ");
                }
                if (len > 10) safe_print("...");
                safe_print("\n");
            }
            ++count;
        }
    }
    safe_print("Total active blocks: ");
    int_to_dec((int64_t)count, type_buf);
    safe_print(type_buf);
    safe_print("\n");
}