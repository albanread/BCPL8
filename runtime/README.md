# BCPL Runtime System

This directory contains the BCPL runtime system, which provides core functionality for both JIT-compiled and standalone-compiled BCPL programs.

## Overview

The BCPL runtime is designed with a unique architecture that allows the same code to be used in two different compilation modes:

1. **JIT Mode**: When the BCPL compiler is used with the `-jit` flag, the runtime is linked directly into the compiler and used to execute code immediately.

2. **Standalone Mode**: When compiling BCPL to standalone executables, the runtime is compiled as a separate C library that is linked with the generated assembly.

## File Structure

- **runtime.h**: C-compatible header file defining the public API
- **runtime_core.inc**: Core runtime functions (WRITES, WRITEN, etc.)
- **runtime_string_utils.inc**: String handling with UTF-8 support
- **runtime_io.inc**: File I/O operations
- **runtime.c**: C implementation for standalone builds (includes main())
- **heap_c_bridge.cpp**: C wrappers for the C++ HeapManager
- **runtime_bridge.cpp**: C++ implementation for JIT mode
- **runtime_test.c**: Test program for the standalone runtime

## Design Approach

The key design principle is the use of `.inc` files as shared implementation that can be included by both C and C++ compilation units. This provides a single source of truth for runtime functions while allowing them to be compiled in different contexts.

### Memory Management

- In JIT mode: Uses the C++ `HeapManager` class
- In standalone mode: Uses a simple malloc-based allocator

### String Handling

The runtime provides full UTF-8 support for BCPL strings, enabling:

- Unicode character handling
- Safe string conversions
- File I/O with proper encoding

## How to Build

### For JIT Mode

The runtime is automatically built as part of the main compiler build:

```sh
./build.sh
```

### For Standalone Mode

The standalone C runtime is built when you compile a BCPL program to a standalone executable:

```sh
./compile_standalone.sh your_program.b
```

This creates an executable that includes the C runtime linked with the compiled BCPL code.

## Extending the Runtime

When adding new runtime functions:

1. Declare them in `runtime.h` with appropriate C linkage
2. Implement them in one of the `.inc` files
3. If they require language-specific features:
   - For C++-only features: Add to `runtime_bridge.cpp`
   - For C-only features: Add directly to `runtime.c`

## Testing

You can test the standalone runtime independently using:

```sh
clang -o runtime_test runtime_test.c runtime.c
./runtime_test
```

This will verify that basic runtime functions work as expected.