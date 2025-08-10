#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'LDR' (Load Register Word, 32-bit) instruction with an unsigned immediate offset.
 * @details
 * This function generates the machine code to load a 32-bit word from memory and
 * zero-extend it into the destination register.
 * The operation is `LDR <Wt>, [<Xn>{, #imm}]`.
 *
 * The encoding follows the "Load/Store Register (unsigned immediate)" format:
 * - **size (bits 31-30)**: `10` for word (32-bit) access.
 * - **Family (bits 29-24)**: `0b111001`.
 * - **L (bit 22)**: `1` for Load.
 * - **imm12 (bits 21-10)**: A 12-bit unsigned byte offset.
 * - **Rn (bits 9-5)**: The base address register.
 * - **Rt (bits 4-0)**: The destination register.
 *
 * @param wt The destination register (e.g., "w0"). The loaded word is zero-extended.
 * @param xn The base address register (e.g., "x1", "sp").
 * @param immediate The unsigned byte offset, in the range [0, 4095].
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or out-of-range immediates.
 */
Instruction Encoder::create_ldr_word_imm(const std::string& wt, const std::string& xn, int immediate) {
    if (immediate < 0 || immediate > 4095) {
        throw std::invalid_argument("Immediate for LDR (word) must be an unsigned 12-bit value [0, 4095].");
    }

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

    auto [rt_num, rt_is_64] = parse_register(wt);
    auto [rn_num, rn_is_64] = parse_register(xn);

    if (!rn_is_64) {
        throw std::invalid_argument("LDR base register must be a 64-bit 'X' register or SP.");
    }
    if (rt_is_64) {
        throw std::invalid_argument("Destination register for LDR (word) must be a 32-bit 'W' register.");
    }

    // Base opcode for LDR (word, unsigned immediate) is 0xB9400000 (size=10)
    BitPatcher patcher(0xB9400000);

    patcher.patch(static_cast<uint32_t>(immediate), 10, 12); // imm12
    patcher.patch(rn_num, 5, 5);                             // Rn
    patcher.patch(rt_num, 0, 5);                             // Rt

    std::string assembly_text = "LDR " + wt + ", [" + xn;
    if (immediate != 0) {
        assembly_text += ", #" + std::to_string(immediate);
    }
    assembly_text += "]";

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::LDR;
    instr.dest_reg = Encoder::get_reg_encoding(wt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}