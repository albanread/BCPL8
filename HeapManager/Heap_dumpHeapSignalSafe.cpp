#include "HeapManager.h"
#include "heap_manager_defs.h"
#include "../SignalSafeUtils.h"
#include <algorithm>
#include <cstdint>
#include <unistd.h> // For write()

//
// ✨ **NEW**: A signal-safe helper to build and print a line of text.
// This acts as a replacement for non-reentrant functions like snprintf.
static void safe_format_line(char* line_buf, size_t buf_size, const char* s1, const char* s2, const char* s3, const char* s4, const char* s5, const char* s6, const char* s7 = "") {
    line_buf[0] = '\0';
    strncat(line_buf, s1, buf_size);
    strncat(line_buf, s2, buf_size - strlen(line_buf));
    strncat(line_buf, s3, buf_size - strlen(line_buf));
    strncat(line_buf, s4, buf_size - strlen(line_buf));
    strncat(line_buf, s5, buf_size - strlen(line_buf));
    strncat(line_buf, s6, buf_size - strlen(line_buf));
    strncat(line_buf, s7, buf_size - strlen(line_buf));
    safe_print(line_buf);
}

void HeapManager::dumpHeapSignalSafe() {
    safe_print("\n=== Heap Allocation Report (v3)   =======================\n");

    char index_buf[64], addr_buf[64], size_buf[64], content_buf[64];
    char line_buf[256]; // A single, large buffer for formatting lines
    int active_blocks = 0;

    for (size_t i = 0; i < MAX_HEAP_BLOCKS; ++i) {
        const auto& block = g_heap_blocks_array[i];

        if (block.type != ALLOC_VEC && block.type != ALLOC_STRING) {
            continue;
        }

        active_blocks++;

        // Convert all numbers to strings first, using their dedicated buffers
        int_to_dec((int)i, index_buf);
        u64_to_hex((uint64_t)(uintptr_t)block.address, addr_buf);
        int_to_dec((int64_t)block.size, size_buf);

        if (block.type == ALLOC_VEC) {
            // ✨ **FIX**: Build the entire line safely before printing
            safe_format_line(line_buf, sizeof(line_buf), "Block ", index_buf, ": Type=Vector, Address=", addr_buf, ", Size=", size_buf, "\n");

            safe_print("  Content: [");
            uint64_t* vec_data = static_cast<uint64_t*>(block.address);
            size_t len = vec_data[0];
            size_t items_to_print = std::min(len, (size_t)8);
            for (size_t j = 0; j < items_to_print; ++j) {
                int_to_dec((int64_t)vec_data[j + 1], content_buf);
                safe_print(content_buf);
                if (j < items_to_print - 1) safe_print(", ");
            }
            if (len > items_to_print) safe_print(" ...");
            safe_print("]\n");

        } else if (block.type == ALLOC_STRING) {
            // ✨ **FIX**: Build the entire line safely before printing
            safe_format_line(line_buf, sizeof(line_buf), "Block ", index_buf, ": Type=String, Address=", addr_buf, ", Size=", size_buf, "\n");

            uint64_t* str_metadata = static_cast<uint64_t*>(block.address);
            size_t len = str_metadata[0];
            uint32_t* str_payload = reinterpret_cast<uint32_t*>(str_metadata + 1);

            safe_print("  Content: \"");
            size_t chars_to_print = std::min(len, (size_t)32);
            for (size_t j = 0; j < chars_to_print; ++j) {
                char utf8_buf[5];
                size_t bytes = safeEncode_utf8_char(str_payload[j], utf8_buf);
                if (bytes > 0) {
                    utf8_buf[bytes] = '\0';
                    safe_print(utf8_buf);
                } else {
                    safe_print("?");
                }
            }
            if (len > chars_to_print) safe_print("...");
            safe_print("\"\n");
        }
    }

    if (active_blocks == 0) {
        safe_print("No active Vector or String allocations found.\n");
    }
    safe_print("\n=== End Allocation Report (v3)      =======================\n");
}
