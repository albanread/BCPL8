#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'SDIV' (Signed Divide) instruction.
 * @details
 * This function generates the 32-bit machine code for a signed division instruction.
 * The instruction has the format: `SDIV <Xd|Wd>, <Xn|Wn>, <Xm|Wm>`.
 * The operation is `$rd = rn / rm$`.
 *
 * The encoding follows the "Data-processing (2 source)" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **S (bit 29)**: Must be 0.
 * - **Family (bits 28-22)**: `0b11010110`.
 * - **Rm (bits 20-16)**: The divisor register `xm`.
 * - **opcode2 (bits 15-10)**: `0b000111` for SDIV.
 * - **Rn (bits 9-5)**: The dividend register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register.
 * @param xn The dividend register.
 * @param xm The divisor register.
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or mismatched sizes.
 */
Instruction Encoder::create_sdiv_reg(const std::string& xd, const std::string& xn, const std::string& xm) {
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
        throw std::invalid_argument("Mismatched register sizes. All operands for SDIV must be the same size.");
    }

    // The base opcode for 32-bit SDIV is 0x1AC01C00.
     // The base opcode for 64-bit SDIV is 0x9AC01C00.
     uint32_t base_opcode = rd_is_64 ? 0x9AC00C00 : 0x1AC00C00;
     BitPatcher patcher(base_opcode);

    patcher.patch(rm_num, 16, 5); // Patch the divisor register.
    patcher.patch(rn_num, 5, 5);  // Patch the dividend register.
    patcher.patch(rd_num, 0, 5);  // Patch the destination register.

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "SDIV " + xd + ", " + xn + ", " + xm;

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::SDIV;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    return instr;
}
