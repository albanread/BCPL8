// runtime/main.c
// Minimal C entry point for standalone BCPL executables

#include <stdio.h>

// Declare the BCPL entry point, which will be provided by the compiled .s file.
// The linker will resolve this reference.
extern int START(void);

// Print runtime metrics (provided by the runtime)

int main(int argc, char* argv[]) {
    // (a) Initialize the runtime environment
    printf("BCPL Runtime initializing...\n");

    // (b) Print a runtime message
    printf("Welcome to the BCPL Standalone Runtime!\n");

    // (c) Call the main BCPL routine
    int result = START();

    // (d) Print runtime metrics
    // print_runtime_metrics();

    return result;
}