#ifndef JIT_DEBUG_INFO_H
#define JIT_DEBUG_INFO_H

#include <string>
#include <vector>
#include <cstdint>
#include <map>

// Represents an entry in a function's stack frame
struct FrameEntry {
    std::string name;
    int offset; // Offset from FP
    int size;   // Size in bytes
    std::string type; // e.g., "local", "saved_reg", "canary", "return_address", "old_fp"
};

// Stores debug information for a compiled function
struct FunctionDebugInfo {
    std::string function_name;
    uint64_t entry_address; // The actual JIT address of the function's entry point
    std::vector<FrameEntry> frame_layout;
};

#endif // JIT_DEBUG_INFO_H
