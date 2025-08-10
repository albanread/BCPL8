#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'ADD (immediate)' instruction.
 * @details
 * This function generates the 32-bit machine code for an ADD instruction
 * that adds an immediate value to a register and stores the result
 * in a destination register.
 * The instruction has the format: `ADD <Xd|Wd>, <Xn|Wn>, #imm`.
 *
 * The encoding follows the "Add/subtract (immediate)" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **op (bit 30)**: 0 (for addition).
 * - **S (bit 29)**: 0 (do not set flags).
 * - **Family (bits 28-24)**: `0b10001`.
 * - **sh (bit 22)**: 0 (for no shift on immediate).
 * - **imm12 (bits 21-10)**: The 12-bit unsigned immediate value.
 * - **Rn (bits 9-5)**: The source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register.
 * @param xn The source register.
 * @param immediate An unsigned 12-bit immediate value [0, 4095].
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers, mismatched sizes, or out-of-range immediates.
 */
Instruction Encoder::create_add_imm(const std::string& xd, const std::string& xn, int immediate) {
    // Ensure all braces are properly closed in the function body.
    // (A) Self-checking: Validate the immediate value.
    if (immediate < 0 || immediate > 4095) {
        throw std::invalid_argument("Immediate for ADD must be an unsigned 12-bit value [0, 4095].");
    }

    // Helper lambda to parse register strings.
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
                if (reg_num > 31) {
                     throw std::out_of_range("Register number out of range for '" + reg_str + "'.");
                }
            } catch (const std::logic_error&) {
                throw std::invalid_argument("Invalid register format: '" + reg_str + "'.");
            }
        }
        return {reg_num, is_64bit};
    };

    // (B) Parse and validate registers.
    auto [rd_num, rd_is_64] = parse_register(xd);
    auto [rn_num, rn_is_64] = parse_register(xn);

    if (rd_is_64 != rn_is_64) {
        throw std::invalid_argument("Mismatched register sizes. Operands for ADD (immediate) must be the same size.");
    }

    // (C) Use BitPatcher to construct the instruction word.
    // Base opcode for 32-bit ADD (imm) is 0x11000000.
    BitPatcher patcher(0x11000000);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    // Patch the immediate value (imm12) into bits 10-21.
    patcher.patch(static_cast<uint32_t>(immediate), 10, 12);

    // Patch the source and destination registers.
    patcher.patch(rn_num, 5, 5);
    patcher.patch(rn_num, 5, 5); // Patch the source register.
    patcher.patch(rd_num, 0, 5); // Patch the destination register.

    // (D) Format the assembly string for the Instruction object.
    std::string assembly_text = "ADD " + xd + ", " + xn + ", #" + std::to_string(immediate);

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::ADD;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    return instr;
}
