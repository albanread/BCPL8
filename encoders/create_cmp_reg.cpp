#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'CMP (register)' instruction.
 * @details
 * This function generates the 32-bit machine code for a CMP instruction,
 * which compares two registers and sets the condition flags based on the result.
 * CMP is an alias for SUBS with the zero register (XZR/WZR) as the destination.
 * The instruction has the format: `CMP <Xn|Wn>, <Xm|Wm>`.
 *
 * The encoding follows the "Data-processing (register)" format for SUBS:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **op (bit 30)**: 1 (for subtraction).
 * - **S  (bit 29)**: 1 (to set flags).
 * - **Family (bits 28-21)**: `0b01011000`.
 * - **Rm (bits 20-16)**: The second source register `xm`.
 * - **Rn (bits 9-5)**: The first source register `xn`.
 * - **Rd (bits 4-0)**: `0b11111` (the zero register).
 *
 * @param xn The first source register to compare (e.g., "x1", "wsp").
 * @param xm The second source register to compare (e.g., "x2", "wzr").
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or mismatched sizes.
 */
Instruction Encoder::create_cmp_reg(const std::string& xn, const std::string& xm) {
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

    // (A) Self-checking: parse and validate registers.
    auto [rn_num, rn_is_64] = parse_register(xn);
    auto [rm_num, rm_is_64] = parse_register(xm);

    if (rn_is_64 != rm_is_64) {
        throw std::invalid_argument("Mismatched register sizes. Operands for CMP (register) must be the same size.");
    }

    // (B) Use BitPatcher to construct the instruction word.
    // The base opcode for a 32-bit SUBS (register) is 0x6B000000.
    BitPatcher patcher(0x6B000000);

    if (rn_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    // Patch the source registers `Rn` and `Rm`.
    patcher.patch(rm_num, 16, 5);
    patcher.patch(rn_num, 5, 5);

    // Patch the destination register `Rd` to be the zero register (31).
    patcher.patch(31, 0, 5);

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "CMP " + xn + ", " + xm;

    // (D) Return the completed Instruction object. No relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::CMP;
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    return instr;
}
