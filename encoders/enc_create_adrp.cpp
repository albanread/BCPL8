#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'ADRP' instruction.
 * @details
 * This function generates the machine code for an ADRP instruction. ADRP
 * calculates the address of the 4KB memory page containing a PC-relative
 * address and writes it to a register. It's the first part of a standard
 * two-instruction sequence for loading a full address, typically followed by
 * an `ADD (immediate)` to add the lower 12 bits.
 *
 * The immediate fields (immlo, immhi) are set to 0, and the instruction is
 * tagged with `RelocationType::PAGE_21_BIT_PC_RELATIVE`. The linker is
 * responsible for calculating the final 21-bit signed page offset from the
 * PC and patching it into the instruction.
 *
 * The encoding follows the "PC-relative addressing" format:
 * - **op (bit 31)**: 1 for ADRP.
 * - **immlo (bits 30-29)**: Low 2 bits of the 21-bit page offset.
 * - **Family (bits 28-24)**: `0b10000`.
 * - **immhi (bits 23-5)**: High 19 bits of the 21-bit page offset.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (must be a 64-bit 'X' register).
 * @param label_name The target label for the address calculation.
 * @return An `Instruction` object with relocation info for the linker.
 * @throw std::invalid_argument if the register is not a 64-bit 'X' register.
 */
Instruction Encoder::create_adrp(const std::string& xd, const std::string& label_name) {
    // Helper lambda to parse the destination register.
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) {
            throw std::invalid_argument("Register string cannot be empty.");
        }
        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);

        // ADRP only operates on 64-bit general-purpose registers.
        if (lower_reg.rfind("x", 0) != 0) {
            throw std::invalid_argument("Invalid register for ADRP: '" + reg_str + "'. Must be an 'X' register.");
        }

        try {
            uint32_t reg_num = std::stoul(lower_reg.substr(1));
            if (reg_num > 30) { // X31 (SP/XZR) is not a valid destination for ADRP.
                throw std::out_of_range("Register X31 (SP/XZR) is not a valid destination for ADRP.");
            }
            return {reg_num, true}; // is_64bit is always true
        } catch (const std::logic_error&) {
            throw std::invalid_argument("Invalid register format for ADRP: '" + reg_str + "'.");
        }
    };

    // (A) Parse and validate the register.
    auto [rd_num, rd_is_64] = parse_register(xd);

    // This check is slightly redundant given the parser's logic, but serves as a safeguard.
    if (!rd_is_64) {
        throw std::invalid_argument("ADRP instruction requires a 64-bit 'X' destination register.");
    }

    // (B) Use BitPatcher. The base for ADRP is 0x90000000.
    // The immediate fields are left as zero for the linker to patch.
    BitPatcher patcher(0x90000000);

    // Patch the destination register.
    patcher.patch(rd_num, 0, 5); // Rd

    // (C) Format the assembly string.
    std::string assembly_text = "ADRP " + xd + ", " + label_name;

    // (D) Return the completed Instruction object with relocation information.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::ADRP;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.relocation = RelocationType::PAGE_21_BIT_PC_RELATIVE;
    instr.target_label = label_name;
    return instr;
}
