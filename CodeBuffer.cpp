#include "CodeBuffer.h"
#include "CodeLister.h"
#include "LabelManager.h"
#include <stdexcept>
#include <cstring>
#include <iostream>
#if defined(__APPLE__)
#include <libkern/OSCacheControl.h>
#endif

// Constructor now pre-allocates memory.
CodeBuffer::CodeBuffer(size_t size, bool enable_tracing) : enable_tracing_(enable_tracing) {
    if (size == 0) {
        throw std::runtime_error("CodeBuffer cannot be initialized with zero size.");
    }
    jit_memory_manager_.allocate(size);
}

void* CodeBuffer::getMemoryPointer() const {
    return jit_memory_manager_.getMemoryPointer();
}

void* CodeBuffer::commit(const std::vector<Instruction>& instructions) {
    void* buffer = jit_memory_manager_.getMemoryPointer();

    // The memory is already populated. We just need to generate the listing for debugging.
    if (enable_tracing_ && !instructions.empty()) {
        CodeLister code_lister;
        std::string listing = code_lister.generate_code_listing(
            instructions, LabelManager::instance().get_defined_labels(), reinterpret_cast<size_t>(buffer)
        );
        std::cout << "\n--- CodeBuffer JIT Listing ---" << listing << "--------------------------\n\n";
    }

    // Crucial steps for making the newly written memory executable by the CPU.
#if defined(__APPLE__)
    // Flush data cache and invalidate instruction cache for the allocated region.
    __asm__ volatile("dmb ish");
    sys_icache_invalidate(buffer, jit_memory_manager_.getAlignedSize());
    // Instruction Synchronization Barrier to flush the pipeline.
    __asm__ volatile("isb");
#endif

    // Make the memory executable.
    jit_memory_manager_.makeExecutable();

    return buffer;
}
