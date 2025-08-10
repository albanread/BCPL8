// runtime_string_utils.inc
// String utilities implementation for BCPL runtime
// This file is included by runtime.c (for standalone builds) and runtime_bridge.cpp (for JIT)
// Do not include this file directly in other source files.

#include <string.h> // for memcpy
#include <stdlib.h> // for malloc, free

// --- UTF-8 encoding/decoding helpers ---

// Encodes a Unicode codepoint as UTF-8
// Returns the number of bytes written
static size_t encode_utf8_char(uint32_t codepoint, char* buffer) {
    if (codepoint < 0x80) {
        // 1-byte sequence (ASCII)
        buffer[0] = (char)codepoint;
        return 1;
    } else if (codepoint < 0x800) {
        // 2-byte sequence
        buffer[0] = (char)(0xC0 | ((codepoint >> 6) & 0x1F));
        buffer[1] = (char)(0x80 | (codepoint & 0x3F));
        return 2;
    } else if (codepoint < 0x10000) {
        // 3-byte sequence
        buffer[0] = (char)(0xE0 | ((codepoint >> 12) & 0x0F));
        buffer[1] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (codepoint & 0x3F));
        return 3;
    } else if (codepoint <= 0x10FFFF) {
        // 4-byte sequence
        buffer[0] = (char)(0xF0 | ((codepoint >> 18) & 0x07));
        buffer[1] = (char)(0x80 | ((codepoint >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((codepoint >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (codepoint & 0x3F));
        return 4;
    }

    // Invalid Unicode codepoint, output replacement character
    buffer[0] = (char)0xEF;
    buffer[1] = (char)0xBF;
    buffer[2] = (char)0xBD; // UTF-8 for U+FFFD (replacement character)
    return 3;
}

// Decodes a UTF-8 sequence to a Unicode codepoint
// Updates the pointer to point to the next character
// Returns the decoded codepoint
static uint32_t decode_utf8_char(const unsigned char** ptr, const unsigned char* end) {
    const unsigned char* p = *ptr;

    if (p >= end) {
        return 0; // End of buffer
    }

    uint32_t c = *p++;

    // Single byte (ASCII)
    if ((c & 0x80) == 0) {
        *ptr = p;
        return c;
    }

    // Multi-byte sequence
    int extra_bytes = 0;
    uint32_t min_code = 0;

    if ((c & 0xE0) == 0xC0) {
        // 2-byte sequence
        extra_bytes = 1;
        min_code = 0x80;
        c &= 0x1F;
    } else if ((c & 0xF0) == 0xE0) {
        // 3-byte sequence
        extra_bytes = 2;
        min_code = 0x800;
        c &= 0x0F;
    } else if ((c & 0xF8) == 0xF0) {
        // 4-byte sequence
        extra_bytes = 3;
        min_code = 0x10000;
        c &= 0x07;
    } else {
        // Invalid UTF-8 sequence, skip this byte
        *ptr = p;
        return 0xFFFD; // Replacement character
    }

    // Read continuation bytes
    for (int i = 0; i < extra_bytes; i++) {
        if (p >= end || (*p & 0xC0) != 0x80) {
            // Incomplete or invalid sequence
            *ptr = p;
            return 0xFFFD; // Replacement character
        }
        c = (c << 6) | (*p++ & 0x3F);
    }

    // Check for overlong encodings and invalid codepoints
    if (c < min_code || (c >= 0xD800 && c <= 0xDFFF) || c > 0x10FFFF) {
        *ptr = p;
        return 0xFFFD; // Replacement character
    }

    *ptr = p;
    return c;
}

void* PACKSTRING(uint32_t* bcpl_string) {
    if (!bcpl_string) return NULL;

    // Calculate required byte length first
    size_t byte_len = 0;
    for (int i = 0; bcpl_string[i] != 0; ++i) {
        uint32_t ch = bcpl_string[i];
        if (ch < 0x80) byte_len += 1;
        else if (ch < 0x800) byte_len += 2;
        else if (ch < 0x10000) byte_len += 3;
        else byte_len += 4;
    }

    // Allocate a byte vector using the C-style wrapper
    uint8_t* packed_vec = (uint8_t*)bcpl_alloc_words(byte_len, "PACKSTRING", "result");
    if (!packed_vec) return NULL;

    // Re-iterate and fill the allocated vector
    char* ptr = (char*)packed_vec;
    for (int i = 0; bcpl_string[i] != 0; ++i) {
        ptr += encode_utf8_char(bcpl_string[i], ptr);
    }

    return packed_vec;
}

uint32_t* UNPACKSTRING(const uint8_t* byte_vector) {
    if (!byte_vector) return NULL;

    uint64_t* vec_header = (uint64_t*)byte_vector - 1;
    size_t byte_len = vec_header[0];

    // Count the number of characters first to allocate correctly
    size_t char_count = 0;
    const unsigned char* p_count = byte_vector;
    const unsigned char* end = byte_vector + byte_len;
    while (p_count < end) {
        decode_utf8_char(&p_count, end);
        char_count++;
    }

    // Allocate the BCPL string using the C-style wrapper
    uint32_t* unpacked_str = (uint32_t*)bcpl_alloc_chars(char_count);
    if (!unpacked_str) return NULL;

    // Re-iterate and decode into the new string
    const unsigned char* p_decode = byte_vector;
    for (size_t i = 0; i < char_count && p_decode < end; ++i) {
        unpacked_str[i] = decode_utf8_char(&p_decode, end);
    }
    unpacked_str[char_count] = 0; // Ensure null termination

    return unpacked_str;
}
