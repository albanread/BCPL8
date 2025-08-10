#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'MUL' (Multiply) instruction.
 * @details
 * This function generates the 32-bit machine code for a multiply instruction.
 * `MUL` is an alias for the `MADD` (Multiply-Add) instruction, where the addend
 * register (`Ra`) is the zero register (WZR/XZR).
 * [cite_start]The operation `MUL <Xd>, <Xn>, <Xm>` is encoded as `MADD <Xd>, <Xn>, <Xm>, <XZR>`[cite: 1].
 *
 * [cite_start]The encoding follows the "Data-processing (3 source)" format for MADD[cite: 1]:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-29)**: `0b01`.
 * - **Family (bits 28-21)**: `0b11011000`.
 * - **Rm (bits 20-16)**: The second source register `xm`.
 * - **o0 (bit 15)**: 0.
 * - **Ra (bits 14-10)**: `0b11111` (the zero register).
 * - **Rn (bits 9-5)**: The first source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0").
 * @param xn The first source register.
 * @param xm The second source register.
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or mismatched sizes.
 */
Instruction Encoder::create_mul_reg(const std::string& xd, const std::string& xn, const std::string& xm) {
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
    auto [rd_num, rd_is_64] = parse_register(xd);
    auto [rn_num, rn_is_64] = parse_register(xn);
    auto [rm_num, rm_is_64] = parse_register(xm);

    if (!(rd_is_64 == rn_is_64 && rn_is_64 == rm_is_64)) {
        throw std::invalid_argument("Mismatched register sizes. All operands for MUL must be the same size.");
    }

    // (B) Use BitPatcher to construct the instruction word.
    // Base opcode for 32-bit MADD is 0x1B000000.
    BitPatcher patcher(0x1B000000);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    // Patch the destination and source registers.
    patcher.patch(rd_num, 0, 5);
    patcher.patch(rn_num, 5, 5);
    patcher.patch(rm_num, 16, 5);

    // Patch the addend register (Ra) to be the zero register (31).
    patcher.patch(31, 10, 5);

    // (C) Format the assembly string.
    std::string assembly_text = "MUL " + xd + ", " + xn + ", " + xm;

    // (D) Return the completed Instruction object. No relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::MUL;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    instr.ra_reg = 31; // XZR/WZR
    return instr;
}
