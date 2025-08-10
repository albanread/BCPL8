#ifndef CODE_BUFFER_H
#define CODE_BUFFER_H

#include "JITMemoryManager.h"
#include "Encoder.h" // For Instruction struct
#include <vector>
#include <cstdint>
#include <memory>

// The CodeBuffer places the binary code into memory for immediate JIT execution.
class CodeBuffer {
    bool enable_tracing_;
public:
    // Constructor now allocates a fixed-size block of memory.
    CodeBuffer(size_t size = 32 * 1024 * 1024, bool enable_tracing = false); // Default to 32MB

    // Commits the machine code to executable memory and returns a function pointer.
    // The instructions vector should be the finalized output from the Linker.
    void* commit(const std::vector<Instruction>& instructions);

    // Returns a pointer to the allocated memory.
    void* getMemoryPointer() const;

private:
    JITMemoryManager jit_memory_manager_;
};

#endif // CODE_BUFFER_H
