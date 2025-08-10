#include "BitPatcher.h"
#include "Encoder.h"
#include <string>

/**
 * @brief Creates a 'MOV SP, X29' instruction to restore the stack pointer.
 * @details
 * This is a common instruction in function epilogues used to restore the stack pointer (SP)
 * from the frame pointer (FP, X29), effectively deallocating the stack frame.
 *
 * This instruction is an alias for `ADD SP, X29, #0`.
 *
 * [cite_start]The encoding follows the "Add/subtract (immediate)" format[cite: 1]:
 * - **sf (bit 31)**: `1` (64-bit).
 * - **op (bit 30)**: `0` (addition).
 * - **S (bit 29)**: `0` (do not set flags).
 * - **Family (bits 28-24)**: `0b10001`.
 * - **imm12 (bits 21-10)**: `0`.
 * - **Rn (bits 9-5)**: `29` (FP, X29).
 * - **Rd (bits 4-0)**: `31` (SP).
 *
 * @return An `Instruction` object for `MOV SP, X29`.
 */
Instruction Encoder::create_mov_sp_fp() {
    // This instruction is an alias for ADD SP, X29, #0.
    // The base opcode for 64-bit ADD (immediate) is 0x91000000.
    BitPatcher patcher(0x91000000);

    // The immediate is 0, so no patch is needed for that field.

    // Patch the source register (Rn) to FP (X29).
    patcher.patch(29, 5, 5);

    // Patch the destination register (Rd) to SP (which is encoded as register 31).
    patcher.patch(31, 0, 5);

    // The canonical assembly text for the alias.
    std::string assembly_text = "MOV SP, X29";

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::MOV;
    instr.dest_reg = 31; // SP
    instr.src_reg1 = 29; // X29 (FP)
    return instr;
}
