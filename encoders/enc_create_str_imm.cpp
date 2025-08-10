#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'STR' (Store Register) instruction with an unsigned immediate offset.
 * @details
 * This function generates the machine code to store a register to memory using a
 * base register and an unsigned, scaled immediate offset.
 * [cite_start]The operation is `STR <Xt>, [<Xn>{, #imm}]`[cite: 88].
 *
 * The encoding follows the "Load/Store Register (unsigned immediate)" format:
 * - **size** (bits 31-30): `11` for 64-bit, `10` for 32-bit.
 * - **Family** (bits 29-24): `0b111001`.
 * - **L** (bit 22): `0` for **Store**.
 * - **imm12** (bits 21-10): A 12-bit unsigned immediate, scaled by the register size.
 * - **Rn** (bits 9-5): The base address register.
 * - **Rt** (bits 4-0): The source register.
 *
 * @param xt The source register to store (e.g., "x0", "w0").
 * @param xn The base address register (e.g., "x2", "sp").
 * @param immediate The unsigned byte offset. For 64-bit stores, this must be a multiple of 8 in the range [0, 32760]. For 32-bit, a multiple of 4 in [0, 16380].
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers, out-of-range/unaligned immediates, or using a 32-bit base register.
 */
Instruction Encoder::create_str_imm(const std::string& xt, const std::string& xn, int immediate, const std::string& variable_name) {
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) {
            throw std::invalid_argument("Register string cannot be empty.");
        }
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

    auto [rt_num, rt_is_64] = parse_register(xt);
    auto [rn_num, rn_is_64] = parse_register(xn);

    if (!rn_is_64) {
        throw std::invalid_argument("STR base register must be a 64-bit 'X' register or SP.");
    }

    uint32_t base_opcode;
    int scale;
    int max_offset;

    if (rt_is_64) { // 64-bit STR
        base_opcode = 0xF9000000;
        scale = 8;
        max_offset = 32760;
    } else { // 32-bit STR
        base_opcode = 0xB9000000;
        scale = 4;
        max_offset = 16380;
    }

    if (immediate < 0 || immediate > max_offset || immediate % scale != 0) {
        throw std::invalid_argument("Invalid immediate for STR. Must be a multiple of " + std::to_string(scale) +
                                  " in range [0, " + std::to_string(max_offset) + "].");
    }

    uint32_t imm12 = (immediate / scale);

    BitPatcher patcher(base_opcode);
    patcher.patch(imm12, 10, 12);
    patcher.patch(rn_num, 5, 5);
    patcher.patch(rt_num, 0, 5);

    std::string assembly_text = "STR " + xt + ", [" + xn;
    if (immediate != 0) {
        assembly_text += ", #" + std::to_string(immediate);
    }
    assembly_text += "]";

    // Append the variable name to the comment if provided
    if (!variable_name.empty() && assembly_text.find(variable_name) == std::string::npos) {
        assembly_text += " ; " + variable_name;
    }

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::STR;
    instr.src_reg1 = Encoder::get_reg_encoding(xt); // The register being stored
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
