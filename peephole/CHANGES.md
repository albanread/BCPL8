# Peephole Optimizer Enhancements

## Modularization

The monolithic PeepholeOptimizer.cpp has been split into multiple focused files in the `peephole` directory:

- `popt_main.cpp` - Core optimizer with constructor and optimize method
- `popt_apply_pass.cpp` - Logic for applying optimization passes
- `popt_helpers.cpp` - Helper methods and utilities
- Various pattern implementations (one per file)

## Configuration Improvements

- Added `max_passes` parameter to `optimize()` method (default: 10)
- Enhanced statistics tracking and reporting
- Improved tracing for better debugging

## New Optimization Patterns

### 1. Multiply by Power of Two

**Pattern**: `MUL Xd, Xn, #power_of_two` → `LSL Xd, Xn, #log2(power_of_two)`

**Example**:
```assembly
; Before
movz x1, #8
mul x0, x2, x1

; After
lsl x0, x2, #3
```

**Benefits**:
- Replaces multi-cycle multiply with single-cycle shift
- Reduces register pressure by eliminating temporary register

### 2. Divide by Power of Two

**Pattern**: `SDIV Xd, Xn, #power_of_two` → `ASR Xd, Xn, #log2(power_of_two)`

**Example**:
```assembly
; Before
movz x1, #4
sdiv x0, x2, x1

; After
asr x0, x2, #2
```

**Benefits**:
- Replaces expensive division with fast arithmetic shift
- Preserves sign behavior for signed division

### 3. Compare Zero Branch Fusion

**Pattern**: `CMP Xn, #0; B.EQ label` → `CBZ Xn, label`

**Example**:
```assembly
; Before
cmp x0, #0
b.eq label1

; After
cbz x0, label1
```

**Benefits**:
- Reduces code size and instruction count
- Improves pipeline efficiency

### 4. ALU Operation Fusion

**Pattern**: `ADD Xd, Xn, #imm1; ADD Xd, Xd, #imm2` → `ADD Xd, Xn, #(imm1+imm2)`

**Example**:
```assembly
; Before
add x0, x1, #100
add x0, x0, #200

; After
add x0, x1, #300
```

**Benefits**:
- Reduces instruction count
- Particularly useful for stack offset calculations
- Better pipeline utilization

### 5. Load-Store Forwarding

**Pattern**: `STR Xs, [Xn, #offset]; ...; LDR Xd, [Xn, #offset]` → `STR Xs, [Xn, #offset]; ...; MOV Xd, Xs`

**Example**:
```assembly
; Before
mov x0, #42
str x0, [sp, #16]
add x1, x2, #5
ldr x3, [sp, #16]

; After
mov x0, #42
str x0, [sp, #16]
add x1, x2, #5
mov x3, x0
```

**Benefits**:
- Eliminates redundant memory accesses
- Reduces memory traffic and latency
- Improves instruction throughput

### 6. Floating-Point Fused Multiply-Add

**Pattern**: 
- `FMUL Vd, Vn, Vm; FADD Vd, Vd, Vz` → `FMADD Vd, Vn, Vm, Vz`
- `FMUL Vd, Vn, Vm; FSUB Vd, Vd, Vz` → `FMSUB Vd, Vn, Vm, Vz`

**Example**:
```assembly
; Before
fmul v0, v1, v2
fadd v0, v0, v3

; After
fmadd v0, v1, v2, v3
```

**Benefits**:
- Reduces instruction count
- Takes advantage of ARM64's dedicated fused multiply-add hardware
- Potentially improves floating-point computation throughput
- Maintains numerical precision better than separate operations

### 7. Conditional Select Optimization

**Pattern**: 
- `CSEL Xd, Xn, #0, cond` → `CSINV Xd, Xn, XZR, !cond`
- `CSEL Xd, #0, Xm, cond` → `CSINV Xd, Xm, XZR, cond`
- `CSEL Xd, Xn, Xn, cond` → `MOV Xd, Xn`

**Example**:
```assembly
; Before
csel x0, x1, #0, eq

; After
csinv x0, x1, xzr, ne
```

**Benefits**:
- Reduces instruction count by eliminating redundant conditional selects
- Uses more specialized conditional instructions when appropriate
- Improves code efficiency and readability
- Better utilizes ARM64 conditional instruction capabilities

## Building and Testing

Use the new `build_peephole.sh` script:

```bash
# Build the modularized optimizer
./build_peephole.sh

# Build and run tests
./build_peephole.sh --run
```

### 8. Bit Field Operations Optimization

**Pattern**: 
- `LSR Xd, Xn, #shift; AND Xd, Xd, #mask` → `UBFX Xd, Xn, #lsb, #width`
- `ASR Xd, Xn, #shift; AND Xd, Xd, #mask` → `SBFX Xd, Xn, #lsb, #width`

**Example**:
```assembly
; Before
lsr x0, x1, #8
and x0, x0, #0xff

; After
ubfx x0, x1, #8, #8
```

**Benefits**:
- Reduces instruction count
- Improves code clarity
- Takes advantage of ARM64's specialized bit field instructions
- Better performance for bit manipulation operations

### 9. Address Generation Optimization

**Pattern**: 
- `ADD Xd, Xn, #imm1; ADD Xd, Xd, Xm` → `ADD Xd, Xn, Xm; ADD Xd, Xd, #imm1` 
- `ADD Xd, Xn, #imm1; ADD Xd, Xd, Xm, LSL #imm2` → `ADD Xd, Xn, Xm, LSL #imm2; ADD Xd, Xd, #imm1`
- `ADD Xd, Xn, Xm; LDR/STR Xt, [Xd, #0]` → `LDR/STR Xt, [Xn, Xm]`
- `ADD Xd, Xn, #imm1; LDR/STR Xt, [Xd, #imm2]` → `LDR/STR Xt, [Xn, #(imm1+imm2)]`

**Example**:
```assembly
; Before
add x0, x1, #100
add x0, x0, x2
ldr x3, [x0]

; After
add x0, x1, x2
add x0, x0, #100
; OR even better:
ldr x3, [x1, x2, #100]
```

**Benefits**:
- Reduces instruction count for address calculations
- Takes advantage of ARM64's flexible addressing modes
- Improves code density and performance
- Particularly beneficial for array indexing and struct field access
- Better pipeline utilization by reducing dependency chains

### 10. Redundant Load Elimination

**Pattern**: `LDR Xd1, [Xn, #offset]; ...; LDR Xd2, [Xn, #offset]` → `LDR Xd1, [Xn, #offset]; ...; MOV Xd2, Xd1`

**Example**:
```assembly
; Before
ldr x0, [sp, #16]   ; Load value from stack into x0
add x3, x4, x5      ; Some intervening instructions
sub x2, x3, #10     ; that don't modify [sp, #16] or x0
ldr x1, [sp, #16]   ; Load the SAME value into x1

; After
ldr x0, [sp, #16]   ; Initial load
add x3, x4, x5      ; Intervening instructions preserved
sub x2, x3, #10
mov x1, x0          ; Replace redundant load with fast register move
```

**Benefits**:
- Eliminates unnecessary memory access operations
- Significantly reduces latency (register moves are much faster than memory loads)
- Particularly beneficial in register-constrained code where values are reloaded
- Complements the existing Load-Store Forwarding optimization
- Helps mitigate memory access bottlenecks

### 11. Identity Operation Elimination

**Pattern**:
- `ADD/SUB Xd, Xn, #0` → `MOV Xd, Xn`
- `MUL/DIV Xd, Xn, #1` → `MOV Xd, Xn`
- `MUL Xd, Xn, #-1` → `NEG Xd, Xn`
- `SUB Xd, Xn, Xn` → `MOV Xd, #0`

**Example**:
```assembly
; Before
add x0, x1, #0     ; Adding zero has no effect
movz w2, #1        ; Load constant 1
mul x3, x4, x2     ; Multiply by 1 has no effect
sub x5, x6, x6     ; Subtracting from itself always gives zero

; After
mov x0, x1         ; Direct register move
mov x3, x4         ; Direct register move
mov x5, #0         ; Set register to zero
```

**Benefits**:
- Eliminates instructions that have no effect on program state
- Reduces instruction count and improves code density
- Replaces complex operations with simpler, faster ones
- Cleans up unnecessary computations often generated by compilers
- Particularly beneficial after other optimizations like constant folding

### 12. Fusing MOV Instructions with ALU Operations

**Pattern**:
- `MOVZ Xn, #imm; <ALU_OP> Xd, Xs, Xn` → `<ALU_OP> Xd, Xs, #imm`

Where `<ALU_OP>` can be ADD, SUB, AND, ORR, EOR, etc.

**Example**:
```assembly
; Before
movz x1, #100       ; Load immediate into a register
add x0, x0, x1      ; Add the register to another

; After
add x0, x0, #100    ; Combine into a single instruction
```

**Benefits**:
- Directly reduces instruction count by eliminating the MOVZ instruction
- Improves execution speed by reducing memory accesses and pipeline pressure
- Particularly effective after constant propagation and inlining
- Reduces register pressure by eliminating temporary registers
- Improves code density and cache utilization

## Future Work

1. Add more specialized patterns for ARM64 architecture
2. Implement common subexpression elimination
3. Improve dead code elimination
4. Add instruction scheduling for better pipeline usage