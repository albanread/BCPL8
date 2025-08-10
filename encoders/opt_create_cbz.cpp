#include <algorithm>
#include <cstdint>
#include "BitPatcher.h"
#include "Encoder.h"
#include <string>
#include <stdexcept>

/**
 * @brief Encodes the ARM64 'CBZ' (Compare and Branch if Zero) instruction.
 * @details
 * This function generates the machine code for a CBZ instruction, which tests
 * the value in a register and branches to a label if the value is zero.
 * The operation is `CBZ <Xt|Wt>, <label>`.
 *
 * The encoding follows the "Compare and branch (immediate)" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **Family (bits 30-24)**: `0b0110100`.
 * - **imm19 (bits 23-5)**: A 19-bit signed PC-relative immediate offset (offset / 4).
 * - **Rt (bits 4-0)**: The source register to test.
 *
 * @param xt The source register to test (e.g., "x0", "w1").
 * @param label_name The target label to branch to.
 * @return An `Instruction` object with relocation information for the linker.
 * @throw std::invalid_argument for invalid register names.
 */
Instruction Encoder::opt_create_cbz(const std::string& xt, const std::string& label_name) {
    // 1. Validate register name and determine size
    uint32_t rt_num = get_reg_encoding(xt);
    bool is_64bit = (xt[0] == 'x' || xt[0] == 'X');

    // 2. Use BitPatcher to construct the instruction word.
    // The immediate field (imm19) is left as zero for the linker to patch.
    // Base opcode for CBZ is 0x34000000.
    BitPatcher patcher(0x34000000);

    if (is_64bit) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(rt_num, 0, 5); // Rt

    // 3. Format the assembly string.
    std::string assembly_text = "CBZ " + xt + ", " + label_name;

    // 4. Return the completed Instruction object with relocation information.
    // The linker will calculate the 19-bit PC-relative offset.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::CBZ;
    instr.src_reg1 = Encoder::get_reg_encoding(xt);
    instr.relocation = RelocationType::PC_RELATIVE_19_BIT_OFFSET;
    instr.target_label = label_name;
    return instr;
}