# Modularized Peephole Optimizer for ARM64

This directory contains the modularized implementation of the peephole optimizer for ARM64 assembly code. The optimizer has been split into multiple files for better maintainability and extensibility.

## File Structure

* `popt_main.cpp` - Main optimizer implementation including constructor and optimize method
* `popt_apply_pass.cpp` - Logic for applying optimization passes
* `popt_helpers.cpp` - Helper methods for tracing and instruction analysis
* `popt_multiply_power_two.cpp` - Pattern for optimizing multiplication by powers of two
* `popt_divide_power_two.cpp` - Pattern for optimizing division by powers of two
* `popt_compare_zero_branch.cpp` - Pattern for fusing compare-zero and branch instructions
* `popt_fuse_alu_operations.cpp` - Pattern for fusing consecutive ALU operations
* `popt_load_store_forwarding.cpp` - Pattern for optimizing redundant memory accesses
* `popt_fused_multiply_add.cpp` - Pattern for fusing floating-point multiply and add/subtract operations
* `popt_conditional_select.cpp` - Pattern for optimizing conditional select instructions
* `popt_bit_field_operations.cpp` - Pattern for optimizing bit field manipulation sequences
* `popt_identity_operation_elimination.cpp` - Pattern for eliminating operations that have no effect (add/sub zero, mul/div one)
* `popt_fuse_mov_alu.cpp` - Pattern for fusing MOV of immediate with subsequent ALU operations


## How to Use

To use the modularized peephole optimizer:

1. Build all files using the build script:
   ```
   ./build_peephole.sh
   ```

2. Run tests to verify the optimizations:
   ```
   ./build_peephole.sh --run
   ```

   Or run the test binary directly:
   ```
   ./peephole_build/bin/peephole_test
   ```

## Adding New Optimization Patterns

To add a new optimization pattern:

1. Create a new file in the `peephole` directory with the pattern `popt_<pattern_name>.cpp`
2. Implement a method in `PeepholeOptimizer` class that creates your pattern
3. Update the constructor in `popt_main.cpp` to add your new pattern

Example template for a new pattern file:

```cpp
#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include <iostream>
#include <algorithm>

/**
 * @brief Creates a pattern for [description]
 * Pattern: [example of pattern]
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createMyNewPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: number of instructions to match
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            // Matcher logic: return true if pattern matches at position pos
            return false;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Transformer logic: return replacement instructions
            return { };
        },
        "Description of optimization"
    );
}
```

## Current Optimization Patterns

### Original Patterns
- Redundant Move Elimination
- Dead Store Elimination
- Redundant Compare Elimination
- Constant Folding
- Basic Strength Reduction

### Advanced Patterns
- **Multiply by Power of Two**: Converts `MUL Xd, Xn, #power_of_two` to `LSL Xd, Xn, #log2(power_of_two)`
- **Divide by Power of Two**: Converts `SDIV Xd, Xn, #power_of_two` to `ASR Xd, Xn, #log2(power_of_two)`
- **Compare Zero Branch Fusion**: Converts `CMP Xn, #0; B.EQ label` to `CBZ Xn, label`
- **ALU Operation Fusion**: Combines `ADD Xd, Xn, #imm1; ADD Xd, Xd, #imm2` to `ADD Xd, Xn, #(imm1+imm2)`
- **Load-Store Forwarding**: Replaces `STR Xs, [Xn, #offset]; ...; LDR Xd, [Xn, #offset]` with `STR Xs, [Xn, #offset]; ...; MOV Xd, Xs` when the memory location is not written between the store and load
- **Floating-Point Fused Multiply-Add**: Transforms `FMUL Vd, Vn, Vm; FADD Vd, Vd, Vz` to `FMADD Vd, Vn, Vm, Vz` and `FMUL Vd, Vn, Vm; FSUB Vd, Vd, Vz` to `FMSUB Vd, Vn, Vm, Vz`
- **Conditional Select Optimization**: Transforms `CSEL Xd, Xn, #0, cond` to `CSINV Xd, Xn, XZR, !cond`, `CSEL Xd, #0, Xm, cond` to `CSINV Xd, Xm, XZR, cond`, and eliminates redundant selects like `CSEL Xd, Xn, Xn, cond` → `MOV Xd, Xn`
- **Bit Field Operations**: Transforms bit manipulation sequences like `LSR Xd, Xn, #shift; AND Xd, Xd, #mask` to dedicated bit field instructions like `UBFX Xd, Xn, #lsb, #width` for more efficient bit extraction
- **Address Generation Optimization**: Transforms complex address calculation sequences into more efficient forms using ARM64's addressing modes, such as converting `ADD Xd, Xn, Xm; LDR Xt, [Xd]` to `LDR Xt, [Xn, Xm]` and merging base+offset calculations
- **Redundant Load Elimination**: Transforms sequences where the same memory location is loaded multiple times into a single load followed by register-to-register moves, such as converting `LDR Xd1, [Xn, #offset]; ...; LDR Xd2, [Xn, #offset]` to `LDR Xd1, [Xn, #offset]; ...; MOV Xd2, Xd1`
- **Identity Operation Elimination**: Removes instructions that have no effect on program state, such as converting `ADD Xd, Xs, #0` to `MOV Xd, Xs`, `MUL/DIV Xd, Xs, #1` to `MOV Xd, Xs`, and `SUB Xd, Xs, Xs` to `MOV Xd, #0`
- **Fusing MOV Instructions with ALU Operations**: Combines a MOV of an immediate value with a subsequent ALU operation that uses that register, such as transforming `MOVZ Xn, #imm; ADD Xd, Xs, Xn` into `ADD Xd, Xs, #imm`


## Future Improvements

Some possible enhancements for the future:

1. Add more architecture-specific optimizations for ARM64
2. Implement common subexpression elimination
3. Expand load/store forwarding optimizations for more complex patterns
4. Implement dead code elimination
5. Add instruction scheduling for better pipeline usage
6. Implement more complex memory addressing optimization patterns
7. Extend redundant load elimination to handle more complex addressing modes