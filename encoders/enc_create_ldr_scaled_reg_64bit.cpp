#include "../Encoder.h"
#include "../BitPatcher.h"
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>

/**
 * @brief Encodes an LDR (Load Register) instruction with a scaled 64-bit register offset.
 * @details
 * This function generates the machine code to load a 64-bit register (Xt) from an
 * address computed by a 64-bit base (Xn) plus a shifted 64-bit index register (Xm).
 * The operation is `LDR Xt, [Xn, Xm, LSL #shift]`.
 *
 * For a 64-bit load with a 64-bit index register, the only valid left shift amount defined by the architecture is 3. A shift of 0 (no shift) is also supported.
 *
 * The encoding follows the "Load/Store Register (register offset)" format:
 * - **size (bits 31-30)**: `11` for 64-bit load.
 * - **Family (bits 29-22)**: `0b11100011`.
 * - **L (bit 22)**: `1` for Load.
 * - **Rm (bits 20-16)**: The index register `xm`.
 * - **option (bits 15-13)**: `0b011` for LSL.
 * - **S (bit 12)**: `1` if shift is applied, `0` otherwise.
 * - **Rn (bits 9-5)**: The base address register `xn`.
 * - **Rt (bits 4-0)**: The destination register `xt`.
 *
 * @param xt The 64-bit destination register (e.g., "x0").
 * @param xn The 64-bit base address register (e.g., "x1", "sp").
 * @param xm The 64-bit index register (e.g., "x2").
 * @param shift The left shift amount. **Must be 0 or 3**.
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or unsupported shift values.
 */
Instruction Encoder::create_ldr_scaled_reg_64bit(const std::string& xt, const std::string& xn, const std::string& xm, int shift) {
    // (A) Validate the shift amount for a 64-bit load with a 64-bit register offset.
    if (shift != 0 && shift != 3) {
        throw std::invalid_argument("Invalid shift for 64-bit LDR with 64-bit register offset. Must be 0 or 3.");
    }
    
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) throw std::invalid_argument("Register string cannot be empty.");
        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);
        bool is_64bit;
        uint32_t reg_num;
        if (lower_reg == "wzr" || lower_reg == "wsp") { is_64bit = false; reg_num = 31;
        } else if (lower_reg == "xzr" || lower_reg == "sp") { is_64bit = true; reg_num = 31;
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

    // (B) Parse and validate registers.
    auto [rt_num, rt_is_64] = parse_register(xt);
    auto [rn_num, rn_is_64] = parse_register(xn);
    auto [rm_num, rm_is_64] = parse_register(xm);

    if (!rt_is_64) {
        throw std::invalid_argument("Destination register for LDR (64-bit) must be an 'X' register.");
    }
    if (!rn_is_64 || !rm_is_64) {
        throw std::invalid_argument("Base and index registers for this LDR variant must be 64-bit 'X' registers.");
    }

    // (C) Use BitPatcher.
    // The base opcode for LDR Xt, [Xn, Xm] with LSL option (but no shift applied yet) is 0xF8606000.
    BitPatcher patcher(0xF8606000);

    // Patch registers.
    patcher.patch(rt_num, 0, 5);  // Rt
    patcher.patch(rn_num, 5, 5);  // Rn
    patcher.patch(rm_num, 16, 5); // Rm

    // Patch the shift enable bit if a shift is required.
    if (shift == 3) {
        patcher.patch(1, 12, 1); // Set the 'S' bit to enable the LSL #3
    }

    // (D) Format the assembly string.
    std::string assembly_text = "LDR " + xt + ", [" + xn + ", " + xm;
    if (shift > 0) {
        assembly_text += ", LSL #" + std::to_string(shift);
    }
    assembly_text += "]";
    
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::LDR;
    instr.dest_reg = Encoder::get_reg_encoding(xt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.src_reg1 = Encoder::get_reg_encoding(xm);
    instr.is_mem_op = true;
    return instr;
}