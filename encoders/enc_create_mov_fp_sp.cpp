#include "BitPatcher.h"
#include "Encoder.h"
#include <string>

/**
 * @brief Creates a 'MOV X29, SP' instruction to set up the frame pointer.
 * @details
 * This is a common instruction in function prologues used to set the frame pointer (FP, X29)
 * to the current stack pointer (SP).
 *
 * This instruction is an alias for `ADD X29, SP, #0`.
 *
 * The encoding follows the "Add/subtract (immediate)" format:
 * - **sf (bit 31)**: `1` (64-bit).
 * - **op (bit 30)**: `0` (addition).
 * - **S (bit 29)**: `0` (do not set flags).
 * - **Family (bits 28-24)**: `0b10001`.
 * - **imm12 (bits 21-10)**: `0`.
 * - **Rn (bits 9-5)**: `31` (SP).
 * - **Rd (bits 4-0)**: `29` (FP).
 *
 * @return An `Instruction` object for `MOV X29, SP`.
 */
Instruction Encoder::create_mov_fp_sp() {
    // This instruction is an alias for ADD X29, SP, #0.
    // The base opcode for 64-bit ADD (immediate) is 0x91000000.
    BitPatcher patcher(0x91000000);

    // The immediate is 0, so no patch is needed for that field.

    // Patch the source register (Rn) to SP (which is encoded as register 31).
    patcher.patch(31, 5, 5);

    // Patch the destination register (Rd) to FP (X29).
    patcher.patch(29, 0, 5);

    // The canonical assembly text for the alias.
    std::string assembly_text = "MOV X29, SP";

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::MOV;
    instr.dest_reg = 29; // X29 (FP)
    instr.src_reg1 = 31; // SP
    return instr;
}
