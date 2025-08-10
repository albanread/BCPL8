// heap_c_bridge.cpp
// C++ to C bridge for heap management in JIT compilation mode
// This file is only compiled in JIT mode and provides C-compatible wrappers
// around the C++ HeapManager class.

#include "runtime.h"
#include "../HeapManager/HeapManager.h"
#include "../HeapManager/heap_manager_defs.h"
#include <cstring>
#include <iostream>

// Keep track of allocations for debugging
#ifdef DEBUG_HEAP
static size_t total_allocations = 0;
static size_t total_bytes = 0;
#endif

extern "C" {

void* bcpl_alloc_words(int64_t num_words, const char* func, const char* var) {
    if (num_words <= 0) {
        std::cerr << "ERROR: Attempted to allocate " << num_words 
                  << " words in " << (func ? func : "unknown") 
                  << " for " << (var ? var : "unknown") << std::endl;
        return nullptr;
    }

    // Use the C++ HeapManager to allocate memory
    void* ptr = HeapManager::getInstance().allocVec(num_words);
    
    #ifdef DEBUG_HEAP
    if (ptr) {
        total_allocations++;
        total_bytes += num_words * sizeof(uint64_t);
        std::cout << "HEAP: Allocated " << num_words << " words (" 
                  << (num_words * sizeof(uint64_t)) << " bytes) in " 
                  << (func ? func : "unknown") << " for " 
                  << (var ? var : "unknown") << " at " << ptr << std::endl;
        std::cout << "HEAP: Total " << total_allocations << " allocations, " 
                  << total_bytes << " bytes" << std::endl;
    } else {
        std::cerr << "HEAP: Allocation failed for " << num_words << " words in " 
                  << (func ? func : "unknown") << " for " 
                  << (var ? var : "unknown") << std::endl;
    }
    #endif

    return ptr;
}

void* bcpl_alloc_chars(int64_t num_chars) {
    if (num_chars < 0) {
        std::cerr << "ERROR: Attempted to allocate string with " 
                  << num_chars << " chars" << std::endl;
        return nullptr;
    }

    // Use the C++ HeapManager to allocate a string
    void* ptr = HeapManager::getInstance().allocString(num_chars);
    
    #ifdef DEBUG_HEAP
    if (ptr) {
        uint32_t* string_data = static_cast<uint32_t*>(ptr);
        total_allocations++;
        // Actual allocation includes the size header and null terminator
        total_bytes += sizeof(uint64_t) + (num_chars + 1) * sizeof(uint32_t);
        std::cout << "HEAP: Allocated string with " << num_chars 
                  << " chars (" << ((num_chars + 1) * sizeof(uint32_t)) 
                  << " bytes) at " << ptr << std::endl;
        std::cout << "HEAP: Total " << total_allocations << " allocations, " 
                  << total_bytes << " bytes" << std::endl;
        
        // Initialize the string with zeros for safety
        std::memset(string_data, 0, (num_chars + 1) * sizeof(uint32_t));
    } else {
        std::cerr << "HEAP: String allocation failed for " 
                  << num_chars << " chars" << std::endl;
    }
    #endif

    return ptr;
}

void bcpl_free(void* ptr) {
    if (!ptr) return;
    
    #ifdef DEBUG_HEAP
    std::cout << "HEAP: Freeing memory at " << ptr << std::endl;
    #endif
    
    HeapManager::getInstance().free(ptr);
}

} // extern "C"