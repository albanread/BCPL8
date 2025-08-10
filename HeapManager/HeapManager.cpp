#include "HeapManager.h"
#include <cstdarg> // For va_list, va_start, va_end
#include <cstdio>  // For vsnprintf
#include <cstdlib> // For std::abort

// Static instance for the singleton
HeapManager* HeapManager::instance = nullptr;

HeapManager& HeapManager::getInstance() {
    if (!HeapManager::instance) {
        HeapManager::instance = new HeapManager();
    }
    return *HeapManager::instance;
}

// Private constructor
HeapManager::HeapManager()
    : totalBytesAllocated(0),
      totalBytesFreed(0),
      totalVectorsAllocated(0),
      totalStringsAllocated(0),
      totalVectorsFreed(0),
      totalStringsFreed(0),
      heapIndex(0), traceEnabled(false) {}

// Private trace log helper
void HeapManager::traceLog(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // For now, just print to stderr. Replace with a more robust logging mechanism if needed.
    if (traceEnabled) {
        fprintf(stderr, "%s", buffer);
    }
}

void HeapManager::setTraceEnabled(bool enabled) {
    traceEnabled = enabled;
}
