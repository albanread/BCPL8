#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'STP' (Store Pair of Registers) instruction with an immediate offset.
 * @details
 * This function generates the machine code to store a pair of registers to memory
 * using a base register and a signed immediate offset.
 * [cite_start]The operation is `STP <Xt1>, <Xt2>, [<Xn>{, #imm}]`[cite: 147].
 *
 * The encoding follows the "Load/Store Pair (signed offset)" format:
 * - **size (bits 31-30)**: `10` for 64-bit, `00` for 32-bit.
 * - **Family (bits 29-23)**: `0b1010010`.
 * - **L (bit 22)**: `0` for **Store**.
 * - **imm7 (bits 21-15)**: A 7-bit signed immediate, scaled by the register size.
 * - **Rt2 (bits 14-10)**: The second source register.
 * - **Rn (bits 9-5)**: The base address register.
 * - **Rt (bits 4-0)**: The first source register.
 *
 * @param xt1 The first source register to store (e.g., "x0", "w0").
 * @param xt2 The second source register to store (e.g., "x1", "w1").
 * @param xn The base address register (e.g., "x2", "sp").
 * @param immediate The signed byte offset. For 64-bit stores, this must be a multiple of 8 in the range [-512, 504]. For 32-bit stores, it must be a multiple of 4 in the range [-256, 252].
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers, mismatched sizes, or out-of-range/unaligned immediates.
 */
Instruction Encoder::create_stp_imm(const std::string& xt1, const std::string& xt2, const std::string& xn, int immediate) {
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) throw std::invalid_argument("Register string cannot be empty.");
        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);
        bool is_64bit;
        uint32_t reg_num;
        if (lower_reg == "wzr") { is_64bit = false; reg_num = 31;
        } else if (lower_reg == "xzr") { is_64bit = true; reg_num = 31;
        } else if (lower_reg == "wsp") { is_64bit = false; reg_num = 31;
        } else if (lower_reg == "sp") { is_64bit = true; reg_num = 31;
        } else {
            char prefix = lower_reg[0];
            if (prefix == 'w') is_64bit = false;
            else if (prefix == 'x') is_64bit = true;
            else throw std::invalid_argument("Invalid register prefix in '" + reg_str + "'. Must be 'w' or 'x'.");
            try {
                reg_num = std::stoul(reg_str.substr(1));
                if (reg_num > 31) throw std::out_of_range("Register number out of range.");
            } catch(...) {
                throw std::invalid_argument("Invalid register format: '" + reg_str + "'.");
            }
        }
        return {reg_num, is_64bit};
    };

    auto [rt1_num, rt1_is_64] = parse_register(xt1);
    auto [rt2_num, rt2_is_64] = parse_register(xt2);
    auto [rn_num, rn_is_64] = parse_register(xn);

    if (rt1_is_64 != rt2_is_64) {
        throw std::invalid_argument("STP registers to be stored must be the same size.");
    }
    if (!rn_is_64) {
        throw std::invalid_argument("STP base register must be a 64-bit 'X' register or SP.");
    }

    uint32_t base_opcode;
    int scale;
    int min_offset, max_offset;

    if (rt1_is_64) { // 64-bit STP
        base_opcode = 0xA9000000;
        scale = 8;
        min_offset = -512;
        max_offset = 504;
    } else { // 32-bit STP
        base_opcode = 0x29000000;
        scale = 4;
        min_offset = -256;
        max_offset = 252;
    }

    if (immediate < min_offset || immediate > max_offset || immediate % scale != 0) {
        throw std::invalid_argument("Invalid immediate for STP. Must be a multiple of " + std::to_string(scale) +
                                  " in range [" + std::to_string(min_offset) + ", " + std::to_string(max_offset) + "].");
    }

    uint32_t imm7 = (immediate / scale) & 0x7F; // Get the 7-bit scaled immediate

    BitPatcher patcher(base_opcode);
    patcher.patch(imm7, 15, 7);
    patcher.patch(rt2_num, 10, 5);
    patcher.patch(rn_num, 5, 5);
    patcher.patch(rt1_num, 0, 5);

    std::string assembly_text = "STP " + xt1 + ", " + xt2 + ", [" + xn;
    if (immediate != 0) {
        assembly_text += ", #" + std::to_string(immediate);
    }
    assembly_text += "]";

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::STP;
    instr.src_reg1 = Encoder::get_reg_encoding(xt1); // First source register
    instr.src_reg2 = Encoder::get_reg_encoding(xt2); // Second source register
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
