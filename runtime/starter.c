#include <stdio.h>

// Declare the external entry point for the compiled BCPL code.
// This function will be located in the .s file generated from your BCPL source.
extern void START(void);

/**
 * @brief The main entry point for the C runtime environment.
 *
 * This function is called by the OS when the program starts. It simply
 * calls the main entry point of the compiled BCPL program ("START").
 */
int main(void) {
    // Call the main entry point of the BCPL program.
    START();

    // Although the BCPL program will likely call EXIT,
    // we return 0 here as a fallback.
    return 0;
}