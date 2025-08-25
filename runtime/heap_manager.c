// heap_manager.c
// Pure C heap implementation for BCPL runtime (no C++ dependencies)

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "runtime.h"

// Allocates a vector of 64-bit words (BCPL vector).
void* BCPL_ALLOC_WORDS(int64_t num_words, const char* func, const char* var) {
    if (num_words <= 0) {
        fprintf(stderr, "ERROR: Attempted to allocate %lld words in %s for %s\n",
                (long long)num_words,
                func ? func : "unknown",
                var ? var : "unknown");
        return NULL;
    }
    size_t total_size = sizeof(uint64_t) + (num_words * sizeof(uint64_t));
    uint64_t* block = (uint64_t*)malloc(total_size);
    if (!block) {
        fprintf(stderr, "ERROR: malloc failed for %lld words in %s for %s\n",
                (long long)num_words,
                func ? func : "unknown",
                var ? var : "unknown");
        return NULL;
    }
    block[0] = num_words;
    return (void*)(block + 1);
}

// Allocates a character string buffer (BCPL string).
void* BCPL_ALLOC_CHARS(int64_t num_chars) {
    if (num_chars < 0) return NULL;
    size_t total_size = sizeof(uint64_t) + ((num_chars + 1) * sizeof(uint32_t)); // +1 for null terminator
    uint64_t* block = (uint64_t*)malloc(total_size);
    if (!block) return NULL;
    block[0] = num_chars;
    uint32_t* payload = (uint32_t*)(block + 1);
    payload[num_chars] = 0; // Null terminate
    return (void*)payload;
}

// Frees memory allocated by BCPL_ALLOC_WORDS or BCPL_ALLOC_CHARS.
void FREEVEC(void* ptr) {
    if (!ptr) return;
    uint64_t* block_start = ((uint64_t*)ptr) - 1;
    free(block_start);
}