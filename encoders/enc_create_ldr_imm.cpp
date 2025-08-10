#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'LDR' (Load Register) instruction with an unsigned immediate offset.
 * @details
 * This function generates the machine code to load a register from memory using a
 * base register and an unsigned, scaled immediate offset.
 * The operation is `LDR <Xt>, [<Xn>{, #imm}]`.
 *
 * The encoding follows the "Load/Store Register (unsigned immediate)" format:
 * - **size (bits 31-30)**: `11` for 64-bit, `10` for 32-bit.
 * - **Family (bits 29-24)**: `0b111001`.
 * - **L (bit 22)**: `1` for Load.
 * - **imm12 (bits 21-10)**: A 12-bit unsigned immediate, scaled by the register size (4 or 8).
 * - **Rn (bits 9-5)**: The base address register.
 * - **Rt (bits 4-0)**: The destination register.
 *
 * @param xt The destination register (e.g., "x0", "w0").
 * @param xn The base address register (e.g., "x2", "sp").
 * @param immediate The unsigned byte offset. For 64-bit loads, this must be a multiple of 8 in the range [0, 32760]. For 32-bit, a multiple of 4 in [0, 16380].
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers, out-of-range/unaligned immediates, or using a 32-bit base register.
 */
Instruction Encoder::create_ldr_imm(const std::string& xt, const std::string& xn, int immediate) {
    // Ensure proper closing of all braces in the function body.
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) throw std::invalid_argument("Register string cannot be empty.");
        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);
        bool is_64bit;
        uint32_t reg_num;
        if (lower_reg == "wzr") { is_64bit = false; reg_num = 31; }
        else if (lower_reg == "xzr") { is_64bit = true; reg_num = 31; }
        else if (lower_reg == "wsp") { is_64bit = false; reg_num = 31; }
        else if (lower_reg == "sp") { is_64bit = true; reg_num = 31; }
        else {
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
    }; // Closing the parse_register lambda

    auto [rt_num, rt_is_64] = parse_register(xt); // Parse destination register
    auto [rn_num, rn_is_64] = parse_register(xn); // Parse base register

    if (!rn_is_64) { // Ensure base register is 64-bit
        throw std::invalid_argument("LDR base register must be a 64-bit 'X' register or SP.");
    }

    uint32_t base_opcode;
    int scale;
    int max_offset;

    if (rt_is_64) { // Handle 64-bit LDR
        base_opcode = 0xF9400000;
        scale = 8;
        max_offset = 32760;
    } else { // Handle 32-bit LDR
        base_opcode = 0xB9400000;
        scale = 4;
        max_offset = 16380;
    }

    // Ensure immediate is aligned and within range
    if (immediate % scale != 0 || immediate < 0 || immediate > max_offset) {
        throw std::invalid_argument("Immediate value out of range or not aligned.");
    }
    
    std::string assembly_text = "LDR " + xt + ", [" + xn;
      if (immediate != 0) {
          assembly_text += ", #" + std::to_string(immediate);
      }
      assembly_text += "]";

    // Construct the instruction
    uint32_t instruction = base_opcode | ((immediate / scale) << 10) | (rn_num << 5) | rt_num;
    Instruction instr(instruction, assembly_text);
    instr.opcode = InstructionDecoder::OpType::LDR;
    instr.dest_reg = Encoder::get_reg_encoding(xt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
