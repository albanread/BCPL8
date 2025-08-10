#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'LDRB' (Load Register Byte) instruction with an unsigned immediate offset.
 * @details
 * This function generates the machine code to load a single byte from memory and
 * zero-extend it into the destination register.
 * The operation is `LDRB <Wt|Xt>, [<Xn>{, #imm}]`.
 *
 * The encoding follows the "Load/Store Register (unsigned immediate)" format:
 * - **size (bits 31-30)**: `00` for byte access.
 * - **Family (bits 29-24)**: `0b111001`.
 * - **L (bit 22)**: `1` for Load.
 * - **imm12 (bits 21-10)**: A 12-bit unsigned byte offset.
 * - **Rn (bits 9-5)**: The base address register.
 * - **Rt (bits 4-0)**: The destination register.
 *
 * @param xt The destination register (e.g., "w0", "x0"). The loaded byte is zero-extended.
 * @param xn The base address register (e.g., "x1", "sp").
 * @param immediate The unsigned byte offset, in the range [0, 4095].
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or out-of-range immediates.
 */
Instruction Encoder::create_ldrb_imm(const std::string& xt, const std::string& xn, int immediate) {
    // (A) Validate the immediate offset.
    if (immediate < 0 || immediate > 4095) {
        throw std::invalid_argument("Immediate for LDRB must be an unsigned 12-bit value [0, 4095].");
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

    auto [rt_num, rt_is_64] = parse_register(xt);
    auto [rn_num, rn_is_64] = parse_register(xn);

    if (!rn_is_64) {
        throw std::invalid_argument("LDRB base register must be a 64-bit 'X' register or SP.");
    }

    // (B) Use BitPatcher. The base opcode for LDRB (unsigned immediate) is 0x39400000.
    BitPatcher patcher(0x39400000);

    // Patch the immediate, base register, and destination register.
    patcher.patch(static_cast<uint32_t>(immediate), 10, 12); // imm12
    patcher.patch(rn_num, 5, 5);                             // Rn
    patcher.patch(rt_num, 0, 5);                             // Rt

    // (C) Format the assembly string.
    std::string assembly_text = "LDRB " + xt + ", [" + xn;
    if (immediate != 0) {
        assembly_text += ", #" + std::to_string(immediate);
    }
    assembly_text += "]";

    // (D) Return the completed Instruction object.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::LDRB;
    instr.dest_reg = Encoder::get_reg_encoding(xt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
