#include "SignalSafeUtils.h"
#include <unistd.h> // For write()
#include <cstring>  // For strlen()

void safe_print(const char* str) {
    if (str) {
        write(STDERR_FILENO, str, strlen(str));
    }
}

void u64_to_hex(uint64_t val, char* buf) {
    buf[0] = '0';
    buf[1] = 'x';
    const char* hex_chars = "0123456789abcdef";
    for (int i = 0; i < 16; ++i) {
        int nibble = (val >> (60 - 4 * i)) & 0xF;
        buf[i + 2] = hex_chars[nibble];
    }
    buf[18] = '\0';
}

// ✨ **FIX**: This is a fully corrected and robust implementation.
void int_to_dec(int64_t val, char* buf) {
    char* p = buf;

    if (val < 0) {
        *p++ = '-';
        val = -val;
    }

    if (val == 0) {
        *p++ = '0';
        *p = '\0';
        return;
    }

    char* start = p;
    while (val > 0) {
        *p++ = '0' + (val % 10);
        val /= 10;
    }
    *p = '\0';

    // Reverse the string of digits
    char* end = p - 1;
    while (start < end) {
        char temp = *start;
        *start++ = *end;
        *end-- = temp;
    }
}

size_t safeEncode_utf8_char(uint32_t char_code, char* buffer) {
    if (char_code < 0x80) { // 1-byte sequence
        buffer[0] = (char)char_code;
        return 1;
    } else if (char_code < 0x800) { // 2-byte sequence
        buffer[0] = (char)(0xC0 | (char_code >> 6));
        buffer[1] = (char)(0x80 | (char_code & 0x3F));
        return 2;
    } else if (char_code < 0x10000) { // 3-byte sequence
        buffer[0] = (char)(0xE0 | (char_code >> 12));
        buffer[1] = (char)(0x80 | ((char_code >> 6) & 0x3F));
        buffer[2] = (char)(0x80 | (char_code & 0x3F));
        return 3;
    } else if (char_code < 0x110000) { // 4-byte sequence
        buffer[0] = (char)(0xF0 | (char_code >> 18));
        buffer[1] = (char)(0x80 | ((char_code >> 12) & 0x3F));
        buffer[2] = (char)(0x80 | ((char_code >> 6) & 0x3F));
        buffer[3] = (char)(0x80 | (char_code & 0x3F));
        return 4;
    }
    return 0; // Invalid Unicode codepoint
}
