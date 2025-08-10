// runtime_bridge.cpp
// C++ runtime bridge for BCPL JIT compilation
// This file links the C-compatible runtime functions to the C++ JIT compiler.

#include "runtime.h"
#include "../HeapManager/HeapManager.h"
#include "../HeapManager/heap_manager_defs.h"
#include <iostream>
#include <string>
#include <cstring>

// In JIT mode, we don't include the heap allocation functions from runtime.c
// as they are provided by heap_c_bridge.cpp. However, we do include all the
// shared implementation files.

// Include the shared implementations
#include "runtime_core.inc"
#include "runtime_string_utils.inc"
#include "runtime_io.inc"

// Additional JIT-specific utilities that can use C++ features


namespace runtime {

// Debugging helper for printing runtime version info
void print_runtime_version() {
    std::cout << "BCPL Runtime v" << BCPL_RUNTIME_VERSION << " (JIT Mode)" << std::endl;
}

// Helper for creating BCPL strings from C++ std::string
uint32_t* create_bcpl_string(const std::string& cpp_string) {
    // Allocate space for the BCPL string
    uint32_t* result = static_cast<uint32_t*>(bcpl_alloc_chars(cpp_string.length()));
    if (!result) return nullptr;
    
    // Convert each character - note this assumes the C++ string is UTF-8
    // A more robust implementation would properly decode UTF-8 sequences
    for (size_t i = 0; i < cpp_string.length(); i++) {
        result[i] = static_cast<uint32_t>(static_cast<unsigned char>(cpp_string[i]));
    }
    
    // Ensure null termination
    result[cpp_string.length()] = 0;
    
    return result;
}

// Helper for converting BCPL strings to C++ std::string
std::string bcpl_string_to_cpp(const uint32_t* bcpl_string) {
    if (!bcpl_string) return "";
    
    std::string result;
    
    // Convert each character to UTF-8
    for (size_t i = 0; bcpl_string[i] != 0; i++) {
        uint32_t ch = bcpl_string[i];
        
        if (ch < 0x80) {
            // ASCII
            result.push_back(static_cast<char>(ch));
        } else if (ch < 0x800) {
            // 2-byte sequence
            result.push_back(static_cast<char>(0xC0 | ((ch >> 6) & 0x1F)));
            result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        } else if (ch < 0x10000) {
            // 3-byte sequence
            result.push_back(static_cast<char>(0xE0 | ((ch >> 12) & 0x0F)));
            result.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        } else {
            // 4-byte sequence
            result.push_back(static_cast<char>(0xF0 | ((ch >> 18) & 0x07)));
            result.push_back(static_cast<char>(0x80 | ((ch >> 12) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | ((ch >> 6) & 0x3F)));
            result.push_back(static_cast<char>(0x80 | (ch & 0x3F)));
        }
    }
    
    return result;
}

// Enhanced I/O utilities that can be used by C++ code in JIT mode

// Log a message to stderr with file and line information
void log_error(const char* file, int line, const char* message) {
    std::cerr << file << ":" << line << ": ERROR: " << message << std::endl;
}

// Write formatted text to stdout (C++ version of WRITEF)
void write_formatted(const char* format, ...) {
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
}

} // namespace runtime
