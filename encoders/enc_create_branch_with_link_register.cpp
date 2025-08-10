#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'BLR' (Branch with Link to Register) instruction.
 * @details
 * This function generates the machine code for a BLR instruction, which performs
 * an indirect function call to the address held in a general-purpose register.
 * [cite_start]It stores the return address (PC+4) in the link register (X30)[cite: 96]. The program
 * [cite_start]counter is then set to the address in the source register (`PC = Xn`)[cite: 99].
 *
 * The encoding follows the "Unconditional branch (register)" format:
 * - **Family (bits 31-10)**: Fixed value of `0b1101011000111111000000`.
 * - [cite_start]**Rn (bits 9-5)**: The source register `xn` holding the target address[cite: 95].
 * - **op (bits 4-0)**: Fixed `0b00000`.
 *
 * @param xn The 64-bit register holding the target function address.
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument if the register is not a 64-bit 'X' register.
 */
Instruction Encoder::create_branch_with_link_register(const std::string& xn) {
    // Helper lambda to parse the source register.
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) {
            throw std::invalid_argument("Register string cannot be empty.");
        }
        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);

        // BLR instruction requires a 64-bit register.
        if (lower_reg.rfind("x", 0) != 0) {
            throw std::invalid_argument("Invalid register for BLR: '" + reg_str + "'. Must be an 'X' register.");
        }

        try {
            uint32_t reg_num = std::stoul(lower_reg.substr(1));
            if (reg_num > 31) {
                throw std::out_of_range("Register number out of range for '" + reg_str + "'.");
            }
            return {reg_num, true}; // is_64bit is always true
        } catch (const std::logic_error&) {
            throw std::invalid_argument("Invalid register format for BLR: '" + reg_str + "'.");
        }
    };

    // (A) Parse and validate the register.
    auto [rn_num, rn_is_64] = parse_register(xn);

    if (!rn_is_64) {
        // This check is slightly redundant given the parser, but serves as defense-in-depth.
        throw std::invalid_argument("BLR instruction requires a 64-bit 'X' source register.");
    }

    // (B) Use BitPatcher. The base opcode for BLR is 0xD63F0000.
    BitPatcher patcher(0xD63F0000);

    // Patch the source register (Rn) into bits 5-9.
    patcher.patch(rn_num, 5, 5);

    // (C) Format the assembly string.
    std::string assembly_text = "BLR " + xn;

    // (D) Return the completed Instruction object.
    // Tagging as 'Jump' can be useful for downstream analysis tools.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::BLR;
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    return instr;
}
