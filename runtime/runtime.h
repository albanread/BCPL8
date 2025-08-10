#ifndef RUNTIME_H
#define RUNTIME_H

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

//=============================================================================
// Runtime version information
//=============================================================================
#define BCPL_RUNTIME_VERSION "1.0.0"

//=============================================================================
// Heap Management (C Bridge to C++ HeapManager)
//=============================================================================

/**
 * Allocates a vector of 64-bit words.
 *
 * @param num_words Number of words to allocate
 * @param func      Name of the function making the allocation (for debugging)
 * @param var       Name of the variable receiving the allocation (for debugging)
 * @return          Pointer to the allocated memory, or NULL on failure
 */
void* bcpl_alloc_words(int64_t num_words, const char* func, const char* var);

/**
 * Allocates a character string buffer.
 *
 * @param num_chars Number of characters (excluding null terminator)
 * @return          Pointer to the allocated string, or NULL on failure
 */
void* bcpl_alloc_chars(int64_t num_chars);

/**
 * Frees memory allocated by bcpl_alloc_words or bcpl_alloc_chars.
 *
 * @param ptr       Pointer to the memory to free
 */
void bcpl_free(void* ptr);

//=============================================================================
// Core I/O and System
//=============================================================================

/**
 * Writes a BCPL string to standard output.
 *
 * @param s Pointer to a null-terminated BCPL string
 */
void WRITES(uint32_t* s);

/**
 * Writes a floating-point number to standard output.
 *
 * @param f The floating-point value to write
 */
void WRITEF(double f);

/**
 * Writes an integer to standard output.
 *
 * @param n The integer to write
 */
void WRITEN(int64_t n);

/**
 * Writes a single character to standard output.
 *
 * @param ch The character code to write
 */
void WRITEC(int64_t ch);

/**
 * Reads a single character from standard input.
 *
 * @return The character read, or -1 on EOF
 */
int64_t RDCH(void);

/**
 * Terminates program execution.
 */
void finish(void);

//=============================================================================
// String Utilities
//=============================================================================

/**
 * Packs a BCPL string into a UTF-8 byte vector.
 *
 * @param bcpl_string Pointer to a null-terminated BCPL string
 * @return            Pointer to the packed UTF-8 byte vector, or NULL on failure
 */
void* PACKSTRING(uint32_t* bcpl_string);

/**
 * Unpacks a UTF-8 byte vector into a BCPL string.
 *
 * @param byte_vector Pointer to a UTF-8 byte vector
 * @return            Pointer to the unpacked BCPL string, or NULL on failure
 */
uint32_t* UNPACKSTRING(const uint8_t* byte_vector);

/**
 * Returns the length of a BCPL string.
 *
 * @param s Pointer to a BCPL string
 * @return  The number of characters in the string
 */
int64_t STRLEN(const uint32_t* s);

/**
 * Compares two BCPL strings.
 *
 * @param s1 Pointer to the first BCPL string
 * @param s2 Pointer to the second BCPL string
 * @return   <0 if s1 < s2, 0 if s1 == s2, >0 if s1 > s2
 */
int64_t STRCMP(const uint32_t* s1, const uint32_t* s2);

/**
 * Copies a BCPL string.
 *
 * @param dst Pointer to the destination BCPL string
 * @param src Pointer to the source BCPL string
 * @return    Pointer to the destination string
 */
uint32_t* STRCOPY(uint32_t* dst, const uint32_t* src);

//=============================================================================
// File I/O
//=============================================================================

/**
 * Reads an entire file into a BCPL string.
 *
 * @param filename_str Pointer to a BCPL string containing the filename
 * @return             Pointer to a BCPL string containing the file contents, or NULL on failure
 */
uint32_t* SLURP(uint32_t* filename_str);

/**
 * Writes a BCPL string to a file.
 *
 * @param bcpl_string  Pointer to a BCPL string to write
 * @param filename_str Pointer to a BCPL string containing the filename
 */
void SPIT(uint32_t* bcpl_string, uint32_t* filename_str);

/**
 * Prints runtime memory allocation metrics.
 * Shows counts of allocations, frees, and memory usage.
 */
void print_runtime_metrics(void);

#ifdef __cplusplus
}
#endif

#endif // RUNTIME_H