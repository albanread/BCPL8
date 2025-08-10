#ifndef SIGNAL_SAFE_UTILS_H
#define SIGNAL_SAFE_UTILS_H

#include <cstdint>
#include <cstddef> // For size_t

void safe_print(const char* str);
void u64_to_hex(uint64_t val, char* buf);
void int_to_dec(int64_t val, char* buf);

 
size_t safeEncode_utf8_char(uint32_t char_code, char* buffer);

#endif // SIGNAL_SAFE_UTILS_H
