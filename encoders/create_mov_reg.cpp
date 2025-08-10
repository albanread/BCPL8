#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'MOV (register)' instruction.
 * @details
 * This function generates the 32-bit machine code to move a value from one
 * register to another. The `MOV` instruction is an alias for `ORR` (Bitwise OR)
 * where the first source register (`Rn`) is the zero register (WZR/XZR).
 * The operation `MOV <Xd>, <Xn>` is encoded as `ORR <Xd>, <XZR/WZR>, <Xn>`.
 *
 * [cite_start]The encoding follows the "Data-processing (register)" format for ORR[cite: 1, 58]:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-24)**: `0b0101010` for the ORR (register) instruction.
 * - **Rm (bits 20-16)**: The source register `xn`.
 * - **Rn (bits 9-5)**: `0b11111` (the zero register).
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0", "wsp").
 * @param xn The source register (e.g., "x1", "wzr").
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or mismatched sizes.
 */
Instruction Encoder::create_mov_reg(const std::string& xd, const std::string& xn) {
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

    if (rd_is_64 != rn_is_64) {
        throw std::invalid_argument("Mismatched register sizes. Operands for MOV (register) must be the same size.");
    }

    // (B) Use BitPatcher to construct the instruction word.
    // Base opcode for 32-bit ORR (register) is 0x2A000000.
    BitPatcher patcher(0x2A000000);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    // Patch the destination register (Rd).
    patcher.patch(rd_num, 0, 5);

    // Patch the source register (Xn) into the Rm field.
    patcher.patch(rn_num, 16, 5);

    // Patch the first source register (Rn) to be the zero register (31).
    patcher.patch(31, 5, 5);

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "MOV " + xd + ", " + xn;

    // (D) Return the completed Instruction object. No relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::MOV;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    return instr;
}
