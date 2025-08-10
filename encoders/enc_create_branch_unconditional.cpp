#include "Encoder.h"
#include <string>

/**
 * @brief Creates an unconditional branch instruction (B) to a label.
 * @details
 * This function generates the machine code for an unconditional branch. The branch
 * target is a PC-relative offset encoded as a 26-bit signed immediate. The ARMv8
 * [cite_start]reference manual describes the operation as `$PC = PC + rel_{27:2}:0_2$`. [cite: 85]
 *
 * The immediate field is set to 0, and the instruction is tagged with
 * `RelocationType::PC_RELATIVE_26_BIT_OFFSET`. The linker is responsible for
 * calculating the final offset from the current program counter (PC) and patching the instruction.
 *
 * The encoding follows the "Unconditional branch (immediate)" format:
 * - **op (bits 31-26)**: `0b000101`.
 * - **imm26 (bits 25-0)**: The 26-bit signed PC-relative immediate (offset / 4).
 *
 * @param label_name The target label.
 * @return An `Instruction` object with relocation info.
 */
Instruction Encoder::create_branch_unconditional(const std::string& label_name) {
    // The base opcode for the B instruction is 0x14000000.
    // The immediate field (bits 0-25) is left as zero for the linker to fill in.
    // No variable parts need to be patched into the encoding at this stage.
    uint32_t encoding = 0x14000000;

    // Format the assembly string.
    std::string assembly_text = "B " + label_name;

    // Return the completed Instruction object with relocation information.
    Instruction instr(encoding, assembly_text);
    instr.opcode = InstructionDecoder::OpType::B;
    instr.relocation = RelocationType::PC_RELATIVE_26_BIT_OFFSET;
    instr.target_label = label_name;
    return instr;
}
