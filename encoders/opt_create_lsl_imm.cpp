#include <cstdint>
#include "BitPatcher.h"
#include "Encoder.h"
#include <string>
#include <stdexcept>
#include <algorithm>
#include <cctype>

/**
 * @brief Encodes the ARM64 'LSL (immediate)' instruction.
 * @details
 * This function generates the 32-bit machine code for an LSL instruction
 * that performs a logical left shift on a register by an immediate amount.
 * The instruction has the format: `LSL <Xd|Wd>, <Xn|Wn>, #<shift_amount>`.
 *
 * This instruction is an alias for `UBFM` (Unsigned Bitfield Move).
 * The encoding follows the "Bitfield" format with the following bitfield layout:
 * - [cite_start]**sf (bit 31)**: 1 for 64-bit, 0 for 32-bit[cite: 299].
 * - [cite_start]**opc (bits 30-29)**: `10` for UBFM[cite: 299].
 * - **N (bit 22)**: Must match `sf`. [cite_start]`1` for 64-bit, `0` for 32-bit[cite: 299].
 * - **immr (bits 21-16)**: The rotate amount. [cite_start]For LSL, this is `(-shift) MOD datasize`[cite: 299].
 * - **imms (bits 15-10)**: The bitmask width. [cite_start]For LSL, this is `(datasize - 1 - shift)`[cite: 299].
 * - [cite_start]**Rn (bits 9-5)**: The source register (`xn`)[cite: 299].
 * - [cite_start]**Rd (bits 4-0)**: The destination register (`xd`)[cite: 299].
 *
 * @param xd The destination register (e.g., "x0", "w1").
 * @param xn The source register to be shifted.
 * @param shift_amount The immediate shift amount (0-63 for 64-bit, 0-31 for 32-bit).
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or out-of-range shift amount.
 */
Instruction Encoder::opt_create_lsl_imm(const std::string& xd, const std::string& xn, int shift_amount) {
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
                if (reg_num > 30) {
                     throw std::out_of_range("Register number out of range for '" + reg_str + "'. Use 'wsp'/'sp' or 'wzr'/'xzr' for register 31.");
                }
            } catch (const std::logic_error&) {
                throw std::invalid_argument("Invalid register format: '" + reg_str + "'.");
            }
        }
        return {reg_num, is_64bit};
    };

    auto [rd_num, rd_is_64] = parse_register(xd);
    auto [rn_num, rn_is_64] = parse_register(xn);

    if (rd_is_64 != rn_is_64) {
        throw std::invalid_argument("Mismatched register sizes. Operands for LSL (immediate) must be the same size.");
    }

    int datasize = rd_is_64 ? 64 : 32;
    if (shift_amount < 0 || shift_amount >= datasize) {
        throw std::invalid_argument("Shift amount for LSL is out of range for the register size.");
    }

    // --- Encoding Logic for LSL as UBFM ---
    uint32_t immr = (datasize - shift_amount) % datasize;
    uint32_t imms = datasize - 1 - shift_amount;
    
    // Base opcode for UBFM is 0x53000000 (for 32-bit)
    BitPatcher patcher(0x53000000);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // sf bit
        patcher.patch(1, 22, 1); // N bit
    }

    patcher.patch(immr, 16, 6); // immr
    patcher.patch(imms, 10, 6); // imms
    patcher.patch(rn_num, 5, 5);  // Rn
    patcher.patch(rd_num, 0, 5);  // Rd
    
    std::string assembly_text = "LSL " + xd + ", " + xn + ", #" + std::to_string(shift_amount);

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::LSL;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.immediate = shift_amount;
    instr.uses_immediate = true;
    return instr;
}