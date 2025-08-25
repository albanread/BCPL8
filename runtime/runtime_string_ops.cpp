#include "runtime.h"
#include "ListDataTypes.h"
#include "heap_interface.h"
#include <string>
#include <vector>
#include <cstring>
#include <cstdint>

// Helper to convert a BCPL string payload to a std::string
static std::string bcpl_to_std_string(const uint32_t* payload) {
    if (!payload) return "";
    std::string result;
    for (size_t i = 0; payload[i] != 0; ++i) {
        result += static_cast<char>(payload[i]); // Assumes ASCII for simplicity
    }
    return result;
}

// Helper to allocate a BCPL string payload from a std::string
static uint32_t* std_string_to_bcpl(const std::string& str) {
    uint32_t* bcpl_str = (uint32_t*)bcpl_alloc_chars(str.length());
    for (size_t i = 0; i < str.length(); ++i) {
        bcpl_str[i] = static_cast<uint32_t>(str[i]);
    }
    bcpl_str[str.length()] = 0;
    return bcpl_str;
}

// SPLIT implementation: returns a ListHeader* of BCPL strings
extern "C" struct ListHeader* BCPL_SPLIT_STRING(uint32_t* source_payload, uint32_t* delimiter_payload) {
    // 1. Create the list to hold the results.
    struct ListHeader* result_list = BCPL_LIST_CREATE_EMPTY();

    // 2. Convert BCPL strings to std::string for easier processing.
    std::string source = bcpl_to_std_string(source_payload);
    std::string delimiter = bcpl_to_std_string(delimiter_payload);

    // 3. Handle edge case of an empty delimiter.
    if (delimiter.empty()) {
        // Split into individual characters
        for (char c : source) {
            std::string char_str(1, c);
            uint32_t* new_bcpl_str_payload = std_string_to_bcpl(char_str);
            // Back up from the payload pointer to get the true base pointer
            void* base_ptr = static_cast<uint64_t*>(static_cast<void*>(new_bcpl_str_payload)) - 1;
            // Append the base pointer, making it consistent with LIST(...)
            BCPL_LIST_APPEND_STRING(result_list, static_cast<uint32_t*>(base_ptr));
        }
        return result_list;
    }

    // 4. Find and split substrings.
    size_t start = 0;
    size_t end = source.find(delimiter);
    while (end != std::string::npos) {
        std::string token = source.substr(start, end - start);
        uint32_t* new_bcpl_str_payload = std_string_to_bcpl(token);
        void* base_ptr = static_cast<uint64_t*>(static_cast<void*>(new_bcpl_str_payload)) - 1;
        BCPL_LIST_APPEND_STRING(result_list, static_cast<uint32_t*>(base_ptr));

        start = end + delimiter.length();
        end = source.find(delimiter, start);
    }

    // 5. Add the final substring after the last delimiter.
    std::string last_token = source.substr(start, std::string::npos);
    uint32_t* new_bcpl_str_payload = std_string_to_bcpl(last_token);
    void* base_ptr = static_cast<uint64_t*>(static_cast<void*>(new_bcpl_str_payload)) - 1;
    BCPL_LIST_APPEND_STRING(result_list, static_cast<uint32_t*>(base_ptr));

    return result_list;
}

// JOIN implementation: returns a BCPL string payload
extern "C" uint32_t* BCPL_JOIN_LIST(struct ListHeader* list_header, uint32_t* delimiter_payload) {
    if (!list_header || !list_header->head) {
        return (uint32_t*)bcpl_alloc_chars(0); // Return a new empty string
    }

    std::string delimiter = bcpl_to_std_string(delimiter_payload);
    size_t delimiter_len = delimiter.length();

    // --- Pass 1: Calculate total size (Corrected) ---
    size_t total_len = 0;
    size_t element_count = 0;
    ListAtom* current = list_header->head;
    while (current) {
        if (current->type != ATOM_STRING) {
            return nullptr; // Error: non-string in list
        }
        
        // FIX: Treat ptr_value as the BASE pointer to the [length][payload] struct.
        uint64_t* base_ptr = (uint64_t*)current->value.ptr_value;
        size_t element_len = base_ptr[0]; // Length is at the start of the struct.

        total_len += element_len;
        ++element_count;
        current = current->next;
    }

    if (element_count > 1) {
        total_len += delimiter_len * (element_count - 1);
    }

    // --- Pass 2: Allocate and build the new string (Corrected) ---
    uint32_t* result_payload = (uint32_t*)bcpl_alloc_chars(total_len);
    uint32_t* cursor = result_payload;

    current = list_header->head;
    while (current) {
        // FIX: Recalculate pointers correctly based on the base pointer.
        uint64_t* base_ptr = (uint64_t*)current->value.ptr_value;
        size_t element_len = base_ptr[0];
        uint32_t* element_payload = (uint32_t*)(base_ptr + 1); // Payload is after the length.

        // Copy string element from the correct payload address.
        std::memcpy(cursor, element_payload, element_len * sizeof(uint32_t));
        cursor += element_len;

        // Add delimiter if not last element.
        if (current->next && delimiter_len > 0) {
            for (size_t i = 0; i < delimiter_len; ++i) {
                *cursor++ = static_cast<uint32_t>(delimiter[i]);
            }
        }
        current = current->next;
    }

    return result_payload;
}