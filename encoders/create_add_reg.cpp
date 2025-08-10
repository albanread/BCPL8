#include "Encoder.h"
#include "BitPatcher.h"
#include <stdexcept>
#include <string>
#include <cctype>
#include <algorithm>
#include <vector>

/**
 * @brief Encodes the ARM64 'ADD (register)' instruction.
 * @details
 * This function generates the 32-bit machine code for an ADD instruction
 * that adds two source registers and places the result in a destination register.
 * The operation has the format: `ADD <Xd|Wd>, <Xn|Wn>, <Xm|Wm>` [cite: 1, 6].
 *
 * The encoding follows the "Data-processing (register)" format with the
 * following bitfield layout:
 * - **sf (bit 31)**: 1 for 64-bit (X regs), 0 for 32-bit (W regs).
 * - **opc (bits 30-24)**: A fixed value of `0b0001011` identifies this as an ADD/SUB register operation.
 * - **Rm (bits 20-16)**: The second general-purpose source register, `xm` [cite: 1].
 * - **Rn (bits 9-5)**: The first general-purpose source register, `xn` [cite: 6, 20].
 * - **Rd (bits 4-0)**: The general-purpose destination register, `xd` [cite: 4].
 *
 * @param xd The destination register (e.g., "x0", "w1").
 * @param xn The first source register (e.g., "x1", "sp").
 * @param xm The second source register (e.g., "x2", "wzr").
 * @return An `Instruction` object containing the encoding and assembly text [cite: 2].
 * @throw std::invalid_argument if register names are invalid or if register sizes are mixed.
 */
Instruction Encoder::create_add_reg(const std::string& xd, const std::string& xn, const std::string& xm) {
    // Helper lambda to parse register strings like "x0", "w1", "sp", "wzr".
    // Returns the register number (0-31) and a boolean indicating if it's a 64-bit register.
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) {
            throw std::invalid_argument("Register string cannot be empty.");
        }

        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);

        bool is_64bit;
        uint32_t reg_num;

        if (lower_reg == "wzr") {
            is_64bit = false;
            reg_num = 31;
        } else if (lower_reg == "xzr") {
            is_64bit = true;
            reg_num = 31;
        } else if (lower_reg == "wsp") {
            is_64bit = false;
            reg_num = 31;
        } else if (lower_reg == "sp") {
            is_64bit = true;
            reg_num = 31;
        } else {
            char prefix = lower_reg[0];
            if (prefix == 'w') {
                is_64bit = false;
            } else if (prefix == 'x') {
                is_64bit = true;
            } else {
                throw std::invalid_argument("Invalid register prefix in '" + reg_str + "'. Must be 'w' or 'x'.");
            }

            try {
                reg_num = std::stoul(reg_str.substr(1));
                if (reg_num > 30) {
                     throw std::out_of_range("Register number out of range for '" + reg_str + "'. Use 'wsp'/'sp' or 'wzr'/'xzr' for register 31.");
                }
            } catch (const std::logic_error&) {
                throw std::invalid_argument("Invalid register format: '" + reg_str + "'.");
            }
        }
        return {reg_num, is_64bit};
    };

    // (A) Perform self-checking by parsing and validating all register arguments first.
    auto [rd_num, rd_is_64] = parse_register(xd);
    auto [rn_num, rn_is_64] = parse_register(xn);
    auto [rm_num, rm_is_64] = parse_register(xm);

    if (!(rd_is_64 == rn_is_64 && rn_is_64 == rm_is_64)) {
        throw std::invalid_argument("Mismatched register sizes. All operands for ADD (register) must be simultaneously 32-bit (W) or 64-bit (X).");
    }

    // (B) Use the BitPatcher to construct the instruction word.
    // Base opcode for 32-bit ADD (register) is 0x0B000000.
    BitPatcher patcher(0x0B000000);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(rd_num, 0, 5);  // Rd
    patcher.patch(rn_num, 5, 5);  // Rn
    patcher.patch(rm_num, 16, 5); // Rm

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "ADD " + xd + ", " + xn + ", " + xm;

    // (D) Return the completed Instruction object, using the constructor from Encoder.h [cite: 2].
    // For this simple instruction, no relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::ADD;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    return instr;
}
