
#ifndef HEAP_MANAGER_H
#define HEAP_MANAGER_H

#include <cstddef> // For size_t
#include <cstdint> // For uint64_t, uint32_t

// Include heap manager definitions
#include "heap_manager_defs.h"

class HeapManager {
private:
    // Internal state for metrics
    size_t totalBytesAllocated;
    size_t totalBytesFreed;
    size_t totalVectorsAllocated;
    size_t totalStringsAllocated;
    size_t totalVectorsFreed;
    size_t totalStringsFreed;
    size_t heapIndex; // Current index for allocation tracking

    // Trace flag
    bool traceEnabled; // Controls whether trace messages are printed

    // Private constructor for singleton pattern
    HeapManager();
    HeapManager(const HeapManager&) = delete;
    HeapManager& operator=(const HeapManager&) = delete;

    // Private helper for logging (if needed outside public interface)
    void traceLog(const char* format, ...);

    // Static instance for singleton
    static HeapManager* instance;

public:
    // Singleton access
    static HeapManager& getInstance();

    // Allocation functions
    void* allocVec(size_t numElements);

    // Setter for traceEnabled
    void setTraceEnabled(bool enabled);
    void* allocString(size_t numChars);

    // Deallocation function
    void free(void* payload);

    // Debugging and metrics
    void dumpHeap() const;
    void dumpHeapSignalSafe(); // Must be truly signal-safe
    void printMetrics() const;
};

#endif // HEAP_MANAGER_H