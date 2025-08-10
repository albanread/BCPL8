// runtime_test.c
// Test program for standalone BCPL runtime
// This is a simple C program that exercises the BCPL runtime functions.

#include "runtime.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("=== BCPL Runtime Test ===\n");
    
    // Test basic output functions
    printf("\nTesting basic output functions:\n");
    WRITES(NULL);  // Should print "(null)"
    
    // Create a BCPL string
    uint32_t* hello_str = (uint32_t*)bcpl_alloc_chars(5);
    hello_str[0] = 'H';
    hello_str[1] = 'e';
    hello_str[2] = 'l';
    hello_str[3] = 'l';
    hello_str[4] = 'o';
    hello_str[5] = 0;  // Null terminator
    
    // Test WRITES
    printf("\nTesting WRITES: ");
    WRITES(hello_str);
    printf("\n");
    
    // Test WRITEN
    printf("Testing WRITEN: ");
    WRITEN(12345);
    printf("\n");
    
    // Test WRITEF
    printf("Testing WRITEF: ");
    WRITEF(3.14159);
    printf("\n");
    
    // Test WRITEC
    printf("Testing WRITEC: ");
    WRITEC('A');
    printf("\n");
    
    // Test string functions
    printf("\nTesting string functions:\n");
    
    // Test STRLEN
    printf("STRLEN(hello_str) = %lld\n", (long long)STRLEN(hello_str));
    
    // Create another BCPL string
    uint32_t* world_str = (uint32_t*)bcpl_alloc_chars(5);
    world_str[0] = 'W';
    world_str[1] = 'o';
    world_str[2] = 'r';
    world_str[3] = 'l';
    world_str[4] = 'd';
    world_str[5] = 0;  // Null terminator
    
    // Test STRCMP
    printf("STRCMP(hello_str, world_str) = %lld\n", (long long)STRCMP(hello_str, world_str));
    printf("STRCMP(hello_str, hello_str) = %lld\n", (long long)STRCMP(hello_str, hello_str));
    
    // Test STRCOPY
    uint32_t* copy_str = (uint32_t*)bcpl_alloc_chars(5);
    STRCOPY(copy_str, hello_str);
    printf("STRCOPY result: ");
    WRITES(copy_str);
    printf("\n");
    
    // Test PACKSTRING and UNPACKSTRING
    printf("\nTesting PACKSTRING/UNPACKSTRING:\n");
    
    // Create a string with some Unicode characters
    uint32_t* unicode_str = (uint32_t*)bcpl_alloc_chars(4);
    unicode_str[0] = 'A';           // ASCII
    unicode_str[1] = 0x03B1;        // Greek alpha
    unicode_str[2] = 0x4F60;        // Chinese ni (you)
    unicode_str[3] = 0x1F600;       // Emoji (smiling face)
    unicode_str[4] = 0;             // Null terminator
    
    printf("Original string (showing only ASCII): ");
    WRITES(unicode_str);
    printf("\n");
    
    // Pack it to UTF-8
    void* packed = PACKSTRING(unicode_str);
    if (!packed) {
        printf("PACKSTRING failed\n");
        return 1;
    }
    
    // Print the packed string (binary representation)
    uint8_t* bytes = (uint8_t*)packed;
    uint64_t* header = (uint64_t*)bytes - 1;
    printf("Packed string (binary, %llu bytes): ", (unsigned long long)header[0]);
    for (size_t i = 0; i < header[0]; i++) {
        printf("%02X ", bytes[i]);
    }
    printf("\n");
    
    // Unpack it back
    uint32_t* unpacked = UNPACKSTRING(bytes);
    if (!unpacked) {
        printf("UNPACKSTRING failed\n");
        return 1;
    }
    
    printf("Unpacked string (showing only ASCII): ");
    WRITES(unpacked);
    printf("\n");
    
    // Verify the unpacked string matches the original
    int match = 1;
    for (int i = 0; i <= 4; i++) {
        if (unicode_str[i] != unpacked[i]) {
            match = 0;
            break;
        }
    }
    printf("Strings match: %s\n", match ? "YES" : "NO");
    
    // Test memory management
    printf("\nTesting memory management:\n");
    
    // Free all allocated memory
    bcpl_free(hello_str);
    bcpl_free(world_str);
    bcpl_free(copy_str);
    bcpl_free(unicode_str);
    bcpl_free(packed);
    bcpl_free(unpacked);
    
    printf("All memory freed successfully\n");
    
    // Test file I/O (if files exist)
    printf("\nTesting file I/O:\n");
    
    // Create a test file name
    uint32_t* test_filename = (uint32_t*)bcpl_alloc_chars(8);
    test_filename[0] = 't';
    test_filename[1] = 'e';
    test_filename[2] = 's';
    test_filename[3] = 't';
    test_filename[4] = '.';
    test_filename[5] = 't';
    test_filename[6] = 'x';
    test_filename[7] = 't';
    test_filename[8] = 0;
    
    // Create content to write
    uint32_t* test_content = (uint32_t*)bcpl_alloc_chars(26);
    for (int i = 0; i < 26; i++) {
        test_content[i] = 'A' + i;
    }
    test_content[26] = 0;
    
    printf("Writing to file: ");
    WRITES(test_filename);
    printf("\n");
    
    // Write to file
    SPIT(test_content, test_filename);
    
    // Read it back
    printf("Reading from file...\n");
    uint32_t* read_content = SLURP(test_filename);
    
    if (read_content) {
        printf("File content: ");
        WRITES(read_content);
        printf("\n");
        bcpl_free(read_content);
    } else {
        printf("Failed to read file\n");
    }
    
    // Clean up
    bcpl_free(test_filename);
    bcpl_free(test_content);
    
    printf("\n=== Runtime Test Complete ===\n");
    return 0;
}