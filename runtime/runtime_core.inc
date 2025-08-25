// runtime_core.inc
// Core implementation of BCPL runtime functions
// This file is included by runtime.c (for standalone builds) and runtime_bridge.cpp (for JIT)
// Do not include this file directly in other source files.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

// UTF-8 output helper for WRITEC
static void write_utf8_char(int64_t ch) {
    if (ch < 0 || ch > 0x10FFFF) {
        // Invalid Unicode codepoint
        putchar('?');
        return;
    }

    if (ch < 0x80) {
        // 1-byte sequence (ASCII)
        putchar((char)ch);
    } else if (ch < 0x800) {
        // 2-byte sequence
        putchar(0xC0 | ((ch >> 6) & 0x1F));
        putchar(0x80 | (ch & 0x3F));
    } else if (ch < 0x10000) {
        // 3-byte sequence
        putchar(0xE0 | ((ch >> 12) & 0x0F));
        putchar(0x80 | ((ch >> 6) & 0x3F));
        putchar(0x80 | (ch & 0x3F));
    } else {
        // 4-byte sequence
        putchar(0xF0 | ((ch >> 18) & 0x07));
        putchar(0x80 | ((ch >> 12) & 0x3F));
        putchar(0x80 | ((ch >> 6) & 0x3F));
        putchar(0x80 | (ch & 0x3F));
    }
}

#include  <ctype.h>
static void get_printable_char(uint32_t code, char* buffer, size_t buffer_size) {
    if (code > 255) { // Not a standard ASCII character
        snprintf(buffer, buffer_size, ".");
        return;
    }

    char c = (char)code;
    switch (c) {
        case '\n': snprintf(buffer, buffer_size, "\\n"); break;
        case '\r': snprintf(buffer, buffer_size, "\\r"); break;
        case '\t': snprintf(buffer, buffer_size, "\\t"); break;
        case '\0': snprintf(buffer, buffer_size, "\\0"); break;
        default:
            if (isprint(c)) {
                snprintf(buffer, buffer_size, "%c", c);
            } else {
                snprintf(buffer, buffer_size, ".");
            }
            break;
    }
}



void debug_WRITES(uint32_t* s) {
    // --- 1. Print Pointer Address and Handle Null ---
    printf("\n--- WRITES_debug ---\n");
    printf("String pointer at: %p\n", (void*)s);
    if (!s) {
        printf("Pointer is NULL.\n--------------------\n");
        fflush(stdout);
        return;
    }

    // --- 2. Read and Print Length ---
    uint32_t len = s[0];
    printf("Reported length:   %u (0x%X) words\n", len, len);

    // --- 3. Prepare for Dump ---
    uint32_t* p = s + 1; // Start of character data
    // Dump up to 16 words or the string's length, whichever is smaller.
    uint32_t words_to_dump = (len < 16) ? len : 16;

    printf("Dumping up to %u content words:\n", words_to_dump);
    printf("Offset | Hex Value  | Char\n");
    printf("-------+------------+------\n");

    // --- 4. Perform Hex Dump ---
    for (uint32_t i = 0; i < words_to_dump; ++i) {
        uint32_t code = p[i];
        char printable_char[5];
        get_printable_char(code, printable_char, sizeof(printable_char));

        printf("[%02u]   | 0x%08X |  %s\n", i, code, printable_char);

        // Stop if we hit a null terminator within the dump limit
        if (code == 0) {
            printf("-> Null terminator found.\n");
            break;
        }
    }

    if (len > words_to_dump) {
        printf("... (omitting %u more words)\n", len - words_to_dump);
    }

    printf("--------------------\n");
    fflush(stdout);
}


/**
 * @brief Prints a BCPL-style string.
 * @param s A pointer to the string data. The first word (length) is skipped, and
 * 32-bit characters are read and printed until a null terminator (0) is found.
 */
void WRITES(uint32_t* s) {
    // 1. Handle a null pointer.
    if (!s) {
        fputs("(null)", stdout);
        fflush(stdout);
        return;
    }

    // 2. Start reading from the second word (s[1]), skipping the length at s[0].
    uint32_t* p = s;

    // 3. Loop until a null terminator is found.
    while (*p != 0) {
        // Directly write the character code. The lexer has already handled escapes.
        write_utf8_char(*p);
        p++; // Move to the next character.
    }

    fflush(stdout);
}





void WRITEF(double f) {
    printf("%g", f);
    fflush(stdout);
}

void WRITEN(int64_t n) {
    printf("%lld", (long long)n);
    fflush(stdout);
}

void WRITEC(int64_t ch) {
    write_utf8_char(ch);
    fflush(stdout);
}

int64_t RDCH(void) {
    // This is a simplified implementation that only handles ASCII.
    // A full implementation would need to decode UTF-8 sequences.
    int c = getchar();
    if (c == EOF) return -1;
    return (int64_t)c;
}

void finish(void) {
    exit(0);
}

int64_t STRLEN(const uint32_t* s) {
    if (!s) return 0;

    int64_t len = 0;
    while (s[len] != 0) {
        len++;
    }

    return len;
}

int64_t STRCMP(const uint32_t* s1, const uint32_t* s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;

    int i = 0;
    while (s1[i] == s2[i]) {
        if (s1[i] == 0) return 0; // Equal strings
        i++;
    }

    return (int64_t)s1[i] - (int64_t)s2[i];
}

uint32_t* STRCOPY(uint32_t* dst, const uint32_t* src) {
    if (!dst) return NULL;
    if (!src) {
        dst[0] = 0; // Empty string if source is NULL
        return dst;
    }

    int i = 0;
    while (src[i] != 0) {
        dst[i] = src[i];
        i++;
    }
    dst[i] = 0; // Add null terminator

    return dst;
}
