#include "HeapManager.h"
#include "heap_manager_defs.h" // For AllocType, HeapBlock, MAX_HEAP_BLOCKS
#include "../SignalSafeUtils.h" // For safe_print and int_to_dec

void HeapManager::printMetrics() const {
    safe_print("\n=== Heap Metrics ===\n");
    char buf[64];
    safe_print("Total Bytes Allocated: ");
    int_to_dec((int64_t)totalBytesAllocated, buf);
    safe_print(buf);
    safe_print("\nTotal Bytes Freed: ");
    int_to_dec((int64_t)totalBytesFreed, buf);
    safe_print(buf);
    safe_print("\nTotal Vectors Allocated: ");
    int_to_dec((int64_t)totalVectorsAllocated, buf);
    safe_print(buf);
    safe_print("\nTotal Strings Allocated: ");
    int_to_dec((int64_t)totalStringsAllocated, buf);
    safe_print(buf);
    safe_print("\nTotal Vectors Freed: ");
    int_to_dec((int64_t)totalVectorsFreed, buf);
    safe_print(buf);
    safe_print("\nTotal Strings Freed: ");
    int_to_dec((int64_t)totalStringsFreed, buf);
    safe_print(buf);
    safe_print("\n");
}