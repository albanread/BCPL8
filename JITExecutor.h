#ifndef JIT_EXECUTOR_H
#define JIT_EXECUTOR_H

#include <cstdint>
#include <cstddef> // For size_t
#include <cstddef> // For size_t

// Define a function pointer type for the JIT-compiled code.
using JITFunc = int64_t (*)();

class JITExecutor {
public:
    explicit JITExecutor(bool debug_mode = false);
    ~JITExecutor();

    int64_t execute(JITFunc func);
    void dump_jit_stack_from_signal(uint64_t sp) const;
    void dump_jit_registers(uint64_t x0, uint64_t d0_bits) const;

private:
    void dump_jit_stack(uint64_t final_sp) const;

    // Base address of the mmap'd stack region.
    void* jit_stack_base = nullptr;
    // Size of the JIT stack.
    static constexpr size_t STACK_SIZE = 8 * 1024 * 1024; // 8 MB
    // Flag to enable/disable debug features.
    bool debug_mode;

    // Member variables are not needed for this approach.
};

#endif // JIT_EXECUTOR_H
