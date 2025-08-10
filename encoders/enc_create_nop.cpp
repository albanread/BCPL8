#include "Encoder.h"
#include <string>

/**
 * @brief Encodes the ARM64 'NOP' (No Operation) instruction.
 * @details
 * This function generates the machine code for a NOP instruction, which performs no operation
 * and is commonly used for instruction alignment or timing purposes.
 *
 * The ARM64 NOP instruction is simply an alias for `HINT #0`.
 * The encoding for NOP is:
 * - **opcode (bits 31-21)**: `0b1101010100000011001`
 * - **imm (bits 20-5)**: 0
 * - **op2 (bits 4-0)**: 0
 *
 * The 32-bit encoding for NOP is 0xD503201F.
 *
 * @return An `Instruction` object containing the encoding and assembly text.
 */
Instruction Encoder::create_nop() {
    // The base opcode for the NOP instruction is 0xD503201F.
    uint32_t encoding = 0xD503201F;

    // Format the assembly string.
    std::string assembly_text = "NOP";

    // Return the completed Instruction object. No relocation is needed.
    Instruction instr(encoding, assembly_text);
    instr.opcode = InstructionDecoder::OpType::NOP;
    return instr;
}