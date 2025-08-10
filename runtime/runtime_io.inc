// runtime_io.inc
// File I/O implementation for BCPL runtime
// This file is included by runtime.c (for standalone builds) and runtime_bridge.cpp (for JIT)
// Do not include this file directly in other source files.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper to convert a BCPL string to a temporary C string for file operations
static char* bcpl_to_c_string(const uint32_t* bcpl_str) {
    if (!bcpl_str) return NULL;

    // Calculate length of BCPL string
    size_t len = 0;
    while (bcpl_str[len] != 0) {
        len++;
    }

    // Allocate a zero-initialized buffer using calloc for safety.
    // This guarantees null termination even if the loop fails.
    char* c_str = (char*)calloc(len * 4 + 1, sizeof(char));
    if (!c_str) return NULL;

    // Convert the string
    size_t pos = 0;
    for (size_t i = 0; i < len; i++) {
        uint32_t codepoint = bcpl_str[i];

        if (codepoint < 0x80) {
            // ASCII
            c_str[pos++] = (char)codepoint;
        } else if (codepoint < 0x800) {
            // 2-byte sequence
            c_str[pos++] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
            c_str[pos++] = (char)(0x80 | (codepoint & 0x3F));
        } else if (codepoint < 0x10000) {
            // 3-byte sequence
            c_str[pos++] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
            c_str[pos++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            c_str[pos++] = (char)(0x80 | (codepoint & 0x3F));
        } else {
            // 4-byte sequence
            c_str[pos++] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
            c_str[pos++] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
            c_str[pos++] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
            c_str[pos++] = (char)(0x80 | (codepoint & 0x3F));
        }
    }
    // Final null terminator is already set by calloc, but we set it again for clarity.
    c_str[pos] = '\0';

    return c_str;
}

uint32_t* SLURP(uint32_t* filename_str) {
    if (!filename_str) return NULL;

    char* c_filename = bcpl_to_c_string(filename_str);
    if (!c_filename) return NULL;

    FILE* file = fopen(c_filename, "rb");
    if (!file) {
        free(c_filename);
        return NULL;
    }

    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);

    if (file_size < 0) {
        fclose(file);
        free(c_filename);
        return NULL;
    }

    // Allocate buffer for file contents
    uint8_t* buffer = (uint8_t*)malloc(file_size + 1); // +1 for a safety null byte
    if (!buffer) {
        fclose(file);
        free(c_filename);
        return NULL;
    }

    // Read file
    size_t bytes_read = fread(buffer, 1, file_size, file);
    buffer[bytes_read] = 0; // Ensure null termination for safety
    fclose(file);
    free(c_filename);

    if (bytes_read != (size_t)file_size) {
        free(buffer);
        return NULL;
    }

    // We'll create a BCPL vector with the file data
    uint8_t* temp_vec = (uint8_t*)bcpl_alloc_words(bytes_read, "SLURP", "temp_buffer");
    if (!temp_vec) {
        free(buffer);
        return NULL;
    }

    // Copy file data to the BCPL vector
    memcpy(temp_vec, buffer, bytes_read);
    free(buffer);

    // Unpack the data into a BCPL string
    uint32_t* result = UNPACKSTRING(temp_vec);

    // Free the temporary vector
    bcpl_free(temp_vec);

    return result;
}


// Paranoid debug version of SPIT

void SPIT(uint32_t* bcpl_string, uint32_t* filename_str) {
    fprintf(stderr, "\n[SPIT DEBUG] =================== NEW SPIT CALL ===================\n");

    if (!bcpl_string || !filename_str) {
        fprintf(stderr, "[SPIT DEBUG] ERROR: One of the input strings is NULL.\n");
        return;
    }

    // --- 1. Inspect the RAW BCPL filename string ---
    fprintf(stderr, "[SPIT DEBUG] 1. Inspecting raw input BCPL filename string:\n");
    fprintf(stderr, "       Address: %p\n", (void*)filename_str);
    fprintf(stderr, "       Content (uint32_t values):\n       ");
    for (int i = 0; i < 256; ++i) { // Limit to 256 chars to prevent infinite loop
        fprintf(stderr, "0x%08X ", filename_str[i]);
        if (filename_str[i] == 0) {
            fprintf(stderr, "\n       -> Null terminator found at index %d.\n", i);
            break;
        }
        if (i == 255) {
            fprintf(stderr, "\n       -> WARNING: No null terminator found in the first 256 characters!\n");
        }
    }

    // --- 2. Convert the string ---
    fprintf(stderr, "[SPIT DEBUG] 2. Calling bcpl_to_c_string to convert filename.\n");
    char* c_filename = bcpl_to_c_string(filename_str);

    if (!c_filename) {
        fprintf(stderr, "[SPIT DEBUG] ERROR: bcpl_to_c_string returned NULL.\n");
        return;
    }
    fprintf(stderr, "[SPIT DEBUG]    -> Converted C Filename (printed with %%s): \"%s\"\n", c_filename);

    // --- 3. Hex Dump the final C string ---
    size_t c_len = strlen(c_filename);
    fprintf(stderr, "[SPIT DEBUG] 3. Hex dump of the final C string (length %zu):\n", c_len);
    fprintf(stderr, "       ");
    // Dump length + 2 bytes to see the null terminator and what's after it
    for (size_t i = 0; i < c_len + 2; ++i) {
        fprintf(stderr, "%02X ", (unsigned char)c_filename[i]);
    }
    fprintf(stderr, "\n");


    // --- 4. Attempt to open file ---
    fprintf(stderr, "[SPIT DEBUG] 4. Calling fopen with the converted filename.\n");
    FILE* file = fopen(c_filename, "wb");
    if (!file) {
        // Use perror to get the system error message
        fprintf(stderr, "[SPIT DEBUG] ERROR: fopen failed. System error: ");
        perror(c_filename);
    } else {
        fprintf(stderr, "[SPIT DEBUG]    -> SUCCESS: File opened for writing.\n");
        // We can skip writing the content for this test.
        fclose(file);
    }


    // --- 5. Cleanup ---
    fprintf(stderr, "[SPIT DEBUG] 5. Cleaning up allocated memory.\n");
    free(c_filename);
    // Let's not worry about the content string for now to isolate the filename issue.
    // bcpl_free(packed_bytes);
    fprintf(stderr, "[SPIT DEBUG] =================== END SPIT CALL =====================\n\n");
}
