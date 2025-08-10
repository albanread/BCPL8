#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'ORR' (Bitwise OR) instruction.
 * @details
 * This function generates the 32-bit machine code for an ORR instruction
 * that performs a bitwise OR between two source registers and stores the
 * result in a destination register.
 * The instruction has the format: `ORR <Xd|Wd>, <Xn|Wn>, <Xm|Wm>`.
 * The logical operation is `$rd = rn | op2$`, where op2 is the `rm` register.
 *
 * The encoding follows the "Data-processing (register)" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-24)**: `0b0101010` for the ORR (register) instruction.
 * - **Rm (bits 20-16)**: The second source register `xm`.
 * - **Rn (bits 9-5)**: The first source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0").
 * @param xn The first source register.
 * @param xm The second source register.
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or mismatched sizes.
 */
Instruction Encoder::create_orr_reg(const std::string& xd, const std::string& xn, const std::string& xm) {
    // Base opcode for 32-bit ORR (register) is 0x2A000000.
    BitPatcher patcher(0x2A000000);

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
        throw std::invalid_argument("Mismatched register sizes. All operands for ORR must have the same size.");
    }

    patcher.patch(rd_num, 0, 5);  // Rd
    patcher.patch(rn_num, 5, 5);  // Rn
    patcher.patch(rm_num, 16, 5); // Rm
    Instruction instr(patcher.get_value(), "ORR " + xd + ", " + xn + ", " + xm);
    instr.opcode = InstructionDecoder::OpType::ORR;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    return instr;
}
