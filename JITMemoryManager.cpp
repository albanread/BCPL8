#include "JITMemoryManager.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include <iostream>

// --- Constructor, Destructor, and Move Semantics (Unchanged) ---
JITMemoryManager::JITMemoryManager()
    : memory_block_(nullptr), allocated_size_(0), aligned_size_(0), is_executable_(false) {}

JITMemoryManager::~JITMemoryManager() { deallocate(); }

JITMemoryManager::JITMemoryManager(JITMemoryManager&& other) noexcept
    : memory_block_(other.memory_block_), allocated_size_(other.allocated_size_),
      aligned_size_(other.aligned_size_), is_executable_(other.is_executable_) {
    other.memory_block_ = nullptr;
    other.allocated_size_ = 0;
    other.aligned_size_ = 0;
    other.is_executable_ = false;
}

JITMemoryManager& JITMemoryManager::operator=(JITMemoryManager&& other) noexcept {
    if (this != &other) {
        deallocate();
        memory_block_ = other.memory_block_;
        allocated_size_ = other.allocated_size_;
        aligned_size_ = other.aligned_size_;
        is_executable_ = other.is_executable_;
        other.memory_block_ = nullptr;
        other.allocated_size_ = 0;
        other.aligned_size_ = 0;
        other.is_executable_ = false;
    }
    return *this;
}

// --- Platform-Specific Helpers ---

size_t JITMemoryManager::get_page_size() {
#ifdef _WIN32
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    return si.dwPageSize;
#else
    return sysconf(_SC_PAGESIZE);
#endif
}

void* JITMemoryManager::platform_allocate(size_t size, size_t& out_aligned_size) {
    if (size == 0) return nullptr;
    size_t page_size = get_page_size();
    out_aligned_size = (size + page_size - 1) & ~(page_size - 1);

#ifdef _WIN32
    return VirtualAlloc(NULL, out_aligned_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    // --- THE FIX IS HERE ---
    // On Apple Silicon macOS, we must use the MAP_JIT flag to get correct CPU behavior
    // for self-modifying/JIT-generated code.
    int flags = MAP_PRIVATE | MAP_ANONYMOUS;
    #if defined(__APPLE__) && defined(__aarch64__)
        flags |= MAP_JIT;
    #endif

    void* ptr = mmap(NULL, out_aligned_size, PROT_READ | PROT_WRITE, flags, -1, 0);
    return (ptr == MAP_FAILED) ? nullptr : ptr;
#endif
}

void JITMemoryManager::platform_deallocate(void* ptr, size_t size) {
    if (ptr == nullptr || size == 0) return;
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#else
    munmap(ptr, size);
#endif
}

void JITMemoryManager::platform_set_permissions(void* ptr, size_t size, bool executable) {
    if (ptr == nullptr || size == 0) return;
#ifdef _WIN32
    DWORD old_protect;
    DWORD new_protect = executable ? PAGE_EXECUTE_READ : PAGE_READWRITE;
    if (!VirtualProtect(ptr, size, new_protect, &old_protect)) {
        throw JITMemoryManagerException("Failed to set memory permissions (VirtualProtect).");
    }
#else
    int prot = PROT_READ | (executable ? PROT_EXEC : PROT_WRITE);
    if (mprotect(ptr, size, prot) == -1) {
        throw JITMemoryManagerException("Failed to set memory permissions (mprotect).");
    }
#endif
}

// --- Public Interface Methods ---

void JITMemoryManager::allocate(size_t size) {
    if (memory_block_ != nullptr) throw JITMemoryManagerException("Memory already allocated.");
    if (size == 0) throw JITMemoryManagerException("Cannot allocate 0 bytes.");

    memory_block_ = platform_allocate(size, this->aligned_size_);
    if (memory_block_ == nullptr) throw JITMemoryManagerException("Failed to allocate memory.");

    allocated_size_ = size;
    is_executable_ = false;
}

void JITMemoryManager::deallocate() {
    if (memory_block_ != nullptr) {
        platform_deallocate(memory_block_, this->aligned_size_);
        memory_block_ = nullptr;
        allocated_size_ = 0;
        aligned_size_ = 0;
        is_executable_ = false;
    }
}

void JITMemoryManager::makeExecutable() {
    if (!memory_block_) throw JITMemoryManagerException("No memory allocated.");
    if (is_executable_) return;
    platform_set_permissions(memory_block_, this->aligned_size_, true);
    is_executable_ = true;
}

void JITMemoryManager::makeWritable() {
    if (!memory_block_) throw JITMemoryManagerException("No memory allocated.");
    if (!is_executable_) return;
    platform_set_permissions(memory_block_, this->aligned_size_, false);
    is_executable_ = false;
}
