#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Creates an ADD (immediate) instruction for the lower 12 bits of a PC-relative address.
 * @details
 * This function creates the second instruction in a standard two-instruction sequence
 * used to load a full 64-bit address into a register. It is designed to follow
 * an `ADRP` instruction.
 *
 * 1. `ADRP Xd, <label>`: Calculates the address of the 4KB page containing the label.
 * 2. `ADD Xd, Xn, #:lo12:<label>`: This instruction adds the 12-bit offset of the
 * [cite_start]label within that page to complete the address calculation[cite: 97].
 *
 * The immediate field in the encoding is set to 0. The function tags the instruction
 * with `RelocationType::ADD_12_BIT_UNSIGNED_OFFSET` so a linker can calculate the
 * true 12-bit offset later and patch the machine code.
 *
 * @param xd The destination register, which will hold the final address.
 * @param xn The source register from the preceding `ADRP` instruction (usually the same as `xd`).
 * @param label_name The target label for the address calculation.
 * @return An `Instruction` object with relocation information for the linker.
 * @throw std::invalid_argument if registers are not 64-bit 'X' registers.
 */
Instruction Encoder::create_add_literal(const std::string& xd, const std::string& xn, const std::string& label_name) {
    // Helper lambda to parse register strings.
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) {
            throw std::invalid_argument("Register string cannot be empty.");
        }
        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);

        bool is_64bit;
        uint32_t reg_num;

        if (lower_reg == "xzr") {
            is_64bit = true;
            reg_num = 31;
        } else if (lower_reg == "sp") {
            is_64bit = true;
            reg_num = 31;
        } else {
            char prefix = lower_reg[0];
            if (prefix == 'x') {
                is_64bit = true;
            } else {
                throw std::invalid_argument("Invalid register prefix in '" + reg_str + "'. Must be 'x' for address calculations.");
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

    // (A) Parse and validate registers.
    auto [rd_num, rd_is_64] = parse_register(xd);
    auto [rn_num, rn_is_64] = parse_register(xn);

    // This operation is exclusively for forming 64-bit addresses.
    if (!rd_is_64 || !rn_is_64) {
        throw std::invalid_argument("Operands for ADD (literal) must be 64-bit 'X' registers.");
    }

    // (B) Use BitPatcher. The base for 64-bit ADD (immediate) is 0x91000000.
    // The immediate field (bits 10-21) is intentionally left as zero for the linker to patch.
    BitPatcher patcher(0x91000000);

    // Patch the registers.
    patcher.patch(rn_num, 5, 5);  // Rn
    patcher.patch(rd_num, 0, 5);  // Rd

    // (C) Format the assembly string to show the common linker relocation directive.
    std::string assembly_text = "ADD " + xd + ", " + xn + ", #:lo12:" + label_name;

    // (D) Return the completed Instruction object with relocation information.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::ADD;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.uses_immediate = true; // Although the immediate is patched later, it's conceptually an immediate operation
    instr.relocation = RelocationType::ADD_12_BIT_UNSIGNED_OFFSET;
    instr.target_label = label_name;
    return instr;
}
