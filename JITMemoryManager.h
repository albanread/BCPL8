#ifndef JIT_MEMORY_MANAGER_H
#define JIT_MEMORY_MANAGER_H

#include <cstddef>
#include <stdexcept>
#include <string>

class JITMemoryManagerException : public std::runtime_error {
public:
    explicit JITMemoryManagerException(const std::string& message)
        : std::runtime_error("JITMemoryManager Error: " + message) {}
};

class JITMemoryManager {
public:
    JITMemoryManager();
    ~JITMemoryManager();

    JITMemoryManager(const JITMemoryManager&) = delete;
    JITMemoryManager& operator=(const JITMemoryManager&) = delete;
    JITMemoryManager(JITMemoryManager&& other) noexcept;
    JITMemoryManager& operator=(JITMemoryManager&& other) noexcept;

    void allocate(size_t size);
    void deallocate();
    void makeExecutable();
    void makeWritable();

    void* getMemoryPointer() const { return memory_block_; }
    size_t getSize() const { return allocated_size_; }

    // --- NEW METHOD ---
    // Returns the full, page-aligned size of the allocation.
    size_t getAlignedSize() const { return aligned_size_; }

    bool isExecutable() const { return is_executable_; }

private:
    void* memory_block_;
    size_t allocated_size_;
    size_t aligned_size_; // <-- ADD THIS MEMBER
    bool is_executable_;

    size_t get_page_size();
    void* platform_allocate(size_t size, size_t& out_aligned_size); // Modified signature
    void platform_deallocate(void* ptr, size_t size);
    void platform_set_permissions(void* ptr, size_t size, bool executable);
};

#endif // JIT_MEMORY_MANAGER_H
