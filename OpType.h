#ifndef OPTYPE_H
#define OPTYPE_H

namespace InstructionDecoder {

// This enum has been expanded with all the types that were causing errors.
enum class OpType {
    UNKNOWN,
    // Standard Ops
    MOV, MOVZ, MOVK, FMOV, MOV_FP_SP, MOV_SP_FP,
    ADD, SUB, SUBS,
    MUL, MADD, FADD, FSUB, FMUL, FMSUB, FMADD,
    DIV, SDIV, FDIV,
    AND, ORR, EOR, BIC,
    CMP, FCMP,
    STR, LDR, LDUR, LDRB, STP, LDP, STR_FP, LDR_FP, STR_WORD, LDR_WORD, LDR_SCALED,
    B, BL, BR, BLR, RET, B_COND, ADRP, ADR,
    NOP, DMB, BRK, SVC, DIRECTIVE,
    // Bitfield & Shift
    LSL, LSR, ASR, UBFX, SBFX, BFI,
    // Conditional
    CSET, CSETM, CSINV,
    CBZ, CBNZ,
    // Conversion
    SCVTF, FCVTZS, FCVTMS,
    // Vector/SIMD
    FMLA_VECTOR, FMUL_VECTOR, ADD_VECTOR, LD1_VECTOR, FADD_VECTOR, MUL_VECTOR,
    // Floating-point Math
    FSQRT
};

} // namespace InstructionDecoder

#endif // OPTYPE_H