#define _DARWIN_C_SOURCE // Required for mmap on macOS
#include "JITExecutor.h"
#include "RuntimeManager.h"
#include <iostream>
#include <iomanip>  // For std::setw and std::setfill
#include <cmath>    // For std::isfinite

#include <cstdint>
#include <cstdio>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <sys/mman.h>

#include "SignalSafeUtils.h"

JITExecutor::JITExecutor(bool debug_mode) : debug_mode(debug_mode) {
    jit_stack_base = mmap(nullptr, STACK_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (jit_stack_base == MAP_FAILED) {
        throw std::runtime_error("Failed to allocate JIT stack.");
    }
    if (debug_mode) {
        std::cout << "JITExecutor: Allocated " << (STACK_SIZE / (1024 * 1024)) << "MB stack at " << jit_stack_base << std::endl;
    }
}

JITExecutor::~JITExecutor() {
    if (jit_stack_base) {
        munmap(jit_stack_base, STACK_SIZE);
        if (debug_mode) {
            std::cout << "JITExecutor: Deallocated JIT stack." << std::endl;
        }
    }
}

int64_t JITExecutor::execute(JITFunc func) {
    uint64_t initial_jit_sp = reinterpret_cast<uint64_t>(jit_stack_base) + STACK_SIZE;
    initial_jit_sp &= ~0xF; // 16-byte alignment

    int64_t jit_result = 0;
    uint64_t final_jit_sp = 0;

    __asm__ volatile(
        // 1. Use x19/x20 to store the host state.
        "mov x19, sp\n"
        "mov x20, lr\n"

        // 2. Switch to the JIT stack and call.
        "mov sp, %2\n"
        "blr %3\n"

        // 3. JIT code has returned. Capture results.
        "mov %0, x0\n"
        "mov %1, sp\n"

        // 4. Restore the host state.
        "mov sp, x19\n"
        "mov lr, x20\n"

        // Define output operands
        : "=r"(jit_result),        // %0
          "=r"(final_jit_sp)       // %1

        // Define input operands
        : "r"(initial_jit_sp),     // %2
          "r"(func)                // %3

        // Define clobbered registers. By including x19 and x20, we tell the
        // compiler it is responsible for saving/restoring them if necessary.
        : "x0", "x1", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "x9",
          "x10", "x11", "x12", "x13", "x14", "x15", "x16", "x17",
          "x19", "x20", "lr", "memory"
    );

    if (debug_mode) {
        dump_jit_stack(final_jit_sp);
    }
    
    // Debug output for floating point operations
    if (RuntimeManager::instance().isTracingEnabled()) {
        std::cout << "JIT Execution completed with result: " << jit_result << std::endl;
        // Display result as float (interpret the bits as double)
        double float_result;
        memcpy(&float_result, &jit_result, sizeof(jit_result));
        if (std::isfinite(float_result)) {
            std::cout << "  Same value as double: " << float_result << std::endl;
        } else {
            std::cout << "  Value is not a valid float" << std::endl;
        }
    }

    return jit_result;
}

void JITExecutor::dump_jit_stack(uint64_t final_sp) const {
    if (!RuntimeManager::instance().isTracingEnabled()) {
        return;
    }
    std::cout << "\n--- JIT Stack Dump (Post-Execution) ---\n";
    auto* stack_base_ptr = reinterpret_cast<uint64_t*>(jit_stack_base);
    auto* stack_top_boundary_ptr = reinterpret_cast<uint64_t*>(static_cast<char*>(jit_stack_base) + STACK_SIZE);
    std::cout << "Final JIT SP (captured): 0x" << std::hex << final_sp << std::dec << "\n";
    std::cout << "JIT Stack Base:          0x" << std::hex << reinterpret_cast<uint64_t>(stack_base_ptr) << std::dec << "\n";
    std::cout << "JIT Stack Top Boundary:  0x" << std::hex << reinterpret_cast<uint64_t>(stack_top_boundary_ptr) << std::dec << "\n";
    constexpr int WORDS_TO_DUMP = 64;
    auto* current_dump_ptr = reinterpret_cast<uint64_t*>(final_sp);
    std::cout << "\n--- JIT Stack Content (from final SP upwards) ---\n";
    std::cout << "+--------------------+--------------------+\n";
    for (int i = 0; i < WORDS_TO_DUMP && current_dump_ptr < stack_top_boundary_ptr; ++i) {
         std::cout << "| 0x" << std::setw(16) << std::setfill('0') << std::hex
                   << reinterpret_cast<uint64_t>(current_dump_ptr)
                   << " | 0x" << std::setw(16) << std::setfill('0') << *current_dump_ptr
                   << std::dec << " |\n";
         current_dump_ptr++;
    }
    std::cout << "+--------------------+--------------------+\n";
    std::cout << "--- End of JIT Stack Dump ---\n\n";
}

void JITExecutor::dump_jit_stack_from_signal(uint64_t sp) const {
    if (!RuntimeManager::instance().isTracingEnabled()) {
        return;
    }
    
    safe_print("\n--- JIT Stack Analysis (from signal) ---\n");
    uint64_t stack_base = reinterpret_cast<uint64_t>(jit_stack_base);
    uint64_t stack_upper_bound = stack_base + STACK_SIZE;
    char addr_buf[20];
    char val_buf[20];
    
    safe_print("Crashing SP: ");
    u64_to_hex(sp, addr_buf);
    safe_print(addr_buf);
    safe_print("\n");
    
    if (sp < stack_base || sp >= stack_upper_bound) {
        safe_print("SP is OUTSIDE the JIT stack range.\n");
        return;
    }
    
    safe_print("SP is INSIDE the JIT stack range. Dumping content around SP.\n");
    
    // Calculate words used
    uint64_t words_used = (stack_upper_bound - sp) / sizeof(uint64_t);
    safe_print("Words used: ");
    int_to_dec((int)words_used, addr_buf);
    safe_print(addr_buf);
    safe_print("\n");
    
    // Print stack boundary information
    safe_print("JIT Stack Base: ");
    u64_to_hex(stack_base, addr_buf);
    safe_print(addr_buf);
    safe_print("\n");
    
    static constexpr int WORDS_TO_DUMP_AROUND_SP = 32;
    uint64_t dump_start_addr = sp - (WORDS_TO_DUMP_AROUND_SP / 2 * 8);
    if (dump_start_addr < stack_base) {
        dump_start_addr = stack_base;
    }
    uint64_t dump_end_addr = sp + (WORDS_TO_DUMP_AROUND_SP / 2 * 8);
    if (dump_end_addr > stack_upper_bound) {
        dump_end_addr = stack_upper_bound;
    }
    auto* current_dump_ptr = reinterpret_cast<uint64_t*>(dump_start_addr);
    safe_print("\n--- JIT Stack Content (around SP) ---\n");
    safe_print("| Address            | Hex Value (64-bit) |\n");
    safe_print("+--------------------+--------------------+\n");
    while (reinterpret_cast<uint64_t>(current_dump_ptr) < dump_end_addr) {
        u64_to_hex(reinterpret_cast<uint64_t>(current_dump_ptr), addr_buf);
        u64_to_hex(*current_dump_ptr, val_buf);
        safe_print("| ");
        safe_print(addr_buf);
        safe_print(" | ");
        safe_print(val_buf);
        safe_print(reinterpret_cast<uint64_t>(current_dump_ptr) == sp ? " | <-- SP\n" : " |\n");
        current_dump_ptr++;
    }
    safe_print("+--------------------+--------------------+\n");
    safe_print("--- End of JIT Stack Analysis ---\n");
}

// Debug helper to print register values
void JITExecutor::dump_jit_registers(uint64_t x0, uint64_t d0_bits) const {
    if (!RuntimeManager::instance().isTracingEnabled()) {
        return;
    }
    
    std::cout << "\n--- JIT Register Debug ---\n";
    std::cout << "X0 (integer): 0x" << std::hex << x0 << " (" << std::dec << x0 << ")" << std::endl;
    
    // Interpret the same bits as double-precision float
    double d0_value;
    memcpy(&d0_value, &d0_bits, sizeof(d0_bits));
    
    std::cout << "D0 (float): ";
    if (std::isfinite(d0_value)) {
        std::cout << d0_value << std::endl;
    } else {
        std::cout << "Invalid/NaN" << std::endl;
    }
    std::cout << "--- End Register Debug ---\n\n";
}
