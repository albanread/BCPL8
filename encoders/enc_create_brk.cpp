#include "BitPatcher.h"
#include "Encoder.h"
#include <string>

/**
 * @brief Encodes the ARM64 'BRK' (Breakpoint) instruction.
 * @details
 * This function generates the machine code for a BRK instruction, which causes a
 * synchronous exception to be taken. It's primarily used for debugging purposes,
 * allowing a debugger to halt execution at a specific point. The 16-bit immediate
 * can be used to pass information to the debug handler.
 *
 * The instruction has the format: `BRK #imm`.
 *
 * The encoding follows the "Exception generation" format:
 * - **Family (bits 31-21)**: `0b11010100001`
 * - **imm16 (bits 20-5)**: The 16-bit immediate value
 * - **op (bits 4-0)**: `0b00000`
 *
 * @param imm The 16-bit immediate value [0, 65535].
 * @return An `Instruction` object containing the encoding and assembly text.
 */
Instruction Encoder::create_brk(uint16_t imm) {
    // The base opcode for the BRK instruction is 0xD4200000.
    BitPatcher patcher(0xD4200000);

    // Patch the 16-bit immediate into bits 5-20.
    patcher.patch(imm, 5, 16);

    // Format the assembly string.
    std::string assembly_text = "BRK #" + std::to_string(imm);

    // Return the completed Instruction object. No relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::BRK;
    instr.immediate = imm;
    instr.uses_immediate = true;
    return instr;
}
