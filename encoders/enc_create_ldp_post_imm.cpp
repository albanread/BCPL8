#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'LDP' (Load Pair) instruction with post-indexing.
 * @details
 * This function generates the machine code to load a pair of registers from the
 * address in a base register, and then update the base register by adding an immediate.
 * The operation is `LDP <Xt1>, <Xt2>, [<Xn>], #imm`.
 *
 * The encoding follows the "Load/Store Pair (post-index)" format:
 * - **size (bits 31-30)**: `10` for 64-bit, `00` for 32-bit.
 * - **Family (bits 29-24)**: `0b101000`.
 * - **L (bit 22)**: `1` for Load.
 * - **imm7 (bits 21-15)**: A 7-bit signed immediate, scaled by the register size.
 * - **Rt2 (bits 14-10)**: The second destination register.
 * - **Rn (bits 9-5)**: The base address register (to be updated).
 * - **Rt (bits 4-0)**: The first destination register.
 *
 * @param xt1 The first destination register (e.g., "x0").
 * @param xt2 The second destination register (e.g., "x1").
 * @param xn The base address register (e.g., "x2", "sp"), which will be updated.
 * @param immediate The signed byte offset to add to the base register after loading. Must be a multiple of the register size (4 or 8).
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers, mismatched sizes, or out-of-range/unaligned immediates.
 */
Instruction Encoder::create_ldp_post_imm(const std::string& xt1, const std::string& xt2, const std::string& xn, int immediate) {
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
        throw std::invalid_argument("LDP registers to be loaded must be the same size.");
    }
    if (!rn_is_64) {
        throw std::invalid_argument("LDP base register must be a 64-bit 'X' register or SP.");
    }

    uint32_t base_opcode;
    int scale;
    int min_offset, max_offset;

    if (rt1_is_64) { // 64-bit LDP
        // Correct base_opcode for 64-bit LDP post-indexed:
        // sf=1, opc=01 (Load), V=0 (GP), fixed(010), P=0, M=0, L=1.
        // This corresponds to a leading pattern that ensures V=0 and correct instruction type.
        base_opcode = 0xA8400000; // This should be 10101000_01000000_00000000_00000000
                                  // sf=1, opc=01, V=0, P=0, M=0, L=1, then rest are part of Imm7 and reg encoding.
                                  // This combines sf=1, opc=01, V=0, 010 (fixed), P=0, M=0, L=1.
                                  // Which is: 1010010001XXXXXXX...
                                  // The 0xA8400000 pattern translates correctly for LDP post-index 64-bit.

        scale = 8;
        min_offset = -512;
        max_offset = 504;
    } else { // 32-bit LDP (W registers)
        base_opcode = 0x28400000; // Corresponding base_opcode for 32-bit LDP post-indexed
        scale = 4;
        min_offset = -256;
        max_offset = 252;
    }

    if (immediate % scale != 0 || immediate < min_offset || immediate > max_offset) {
        throw std::invalid_argument("LDP immediate offset must be a multiple of " + std::to_string(scale) +
                                    " and within range [" + std::to_string(min_offset) + ", " + std::to_string(max_offset) + "].");
    }

    uint32_t imm7 = (static_cast<uint32_t>(immediate) / scale) & 0x7F; // Mask to 7 bits

    uint32_t instruction_word = base_opcode |
                                (imm7 << 15) | // imm7 bits 21-15
                                (rt2_num << 10) | // Rt2 bits 14-10
                                (rn_num << 5) | // Rn bits 9-5
                                rt1_num; // Rt bits 4-0

    Instruction instr(instruction_word, "LDP " + xt1 + ", " + xt2 + ", [" + xn + "], #" + std::to_string(immediate));
    instr.opcode = InstructionDecoder::OpType::LDP;
    instr.dest_reg = Encoder::get_reg_encoding(xt1); // First destination register
    instr.src_reg1 = Encoder::get_reg_encoding(xt2); // Second destination register
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
