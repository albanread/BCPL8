#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'CMP (immediate)' instruction.
 * @details
 * This function generates the 32-bit machine code for a CMP instruction,
 * which compares a register with an immediate value and sets the condition flags.
 * CMP is an alias for SUBS with the zero register (XZR/WZR) as the destination.
 * The instruction has the format: `CMP <Xn|Wn>, #imm`.
 *
 * [cite_start]The encoding follows the "Add/subtract (immediate)" format[cite: 1]:
 * - **sf (bit 31)**: 1 for 64-bit (Xn), 0 for 32-bit (Wn).
 * - **op (bit 30)**: 1 (for subtraction).
 * - **S  (bit 29)**: 1 (to set flags).
 * - **Family (bits 28-24)**: `0b10001`.
 * - **sh (bit 22)**: 0 (for no shift on immediate).
 * - **imm12 (bits 21-10)**: The 12-bit unsigned immediate value.
 * - **Rn (bits 9-5)**: The source register `xn`.
 * - **Rd (bits 4-0)**: `0b11111` (the zero register).
 *
 * @param xn The source register to compare (e.g., "x1", "w5", "sp").
 * @param immediate An unsigned 12-bit immediate value [0, 4095].
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or out-of-range immediates.
 */
Instruction Encoder::create_cmp_imm(const std::string& xn, int immediate) {
    // (A) Self-checking: Validate the immediate value.
    if (immediate < 0 || immediate > 4095) {
        throw std::invalid_argument("Immediate for CMP must be an unsigned 12-bit value [0, 4095].");
    }

    // Helper lambda to parse the source register string.
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) {
            throw std::invalid_argument("Register string cannot be empty.");
        }

        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);

        bool is_64bit;
        uint32_t reg_num;

        if (lower_reg == "wsp") {
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
                if (reg_num > 31) { // This will catch invalid numbers but allow x31 for XZR
                     throw std::out_of_range("Register number out of range for '" + reg_str + "'.");
                }
            } catch (const std::logic_error&) {
                throw std::invalid_argument("Invalid register format: '" + reg_str + "'.");
            }
        }
        return {reg_num, is_64bit};
    };

    auto [rn_num, rn_is_64] = parse_register(xn);

    // (B) Use the BitPatcher to construct the instruction word.
    // The base opcode for a 32-bit SUBS (imm) is 0x71000000.
    BitPatcher patcher(0x71000000);

    if (rn_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    // Patch the immediate value (imm12) into bits 10-21.
    patcher.patch(static_cast<uint32_t>(immediate), 10, 12);

    // Patch the source register (Rn) into bits 5-9.
    patcher.patch(rn_num, 5, 5);

    // Patch the destination register (Rd) to be the zero register (31).
    patcher.patch(31, 0, 5);

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "CMP " + xn + ", #" + std::to_string(immediate);

    // (D) Return the completed Instruction object. No relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::CMP;
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    return instr;
}
