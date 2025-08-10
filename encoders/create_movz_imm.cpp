#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'MOVZ' (Move with Zero) instruction.
 * @details
 * This function generates the 32-bit machine code to move a 16-bit immediate
 * into a register, zeroing the other bits. The immediate can be shifted to
 * align with any 16-bit boundary in the register.
 * [cite_start]The instruction has the format: `MOVZ <Xd|Wd>, #imm{, LSL #shift}`[cite: 43].
 * [cite_start]The operation is `$rd = i^ø << sh$`, meaning the immediate is zero-extended and shifted[cite: 44].
 *
 * The encoding follows the "Move wide (immediate)" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-29)**: `0b10` for MOVZ.
 * - **Family (bits 28-23)**: `0b100101`.
 * - **hw (bits 22-21)**: The shift amount, encoded as `shift / 16`.
 * - **imm16 (bits 20-5)**: The 16-bit immediate value.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0", "w1").
 * @param immediate The 16-bit immediate value to move.
 * @param shift The left shift to apply. Must be 0, 16, 32, or 48.
 * @param rel The relocation type for the instruction, used by the linker.
 * @param target The target symbol for relocation.
 * @return An `Instruction` object containing the encoding and other metadata.
 * @throw std::invalid_argument for invalid registers or invalid shift values.
 */
Instruction Encoder::create_movz_imm(const std::string& xd, uint16_t immediate, int shift, RelocationType rel, const std::string& target) {
    // (A) Self-checking: Validate the shift value.
    if (shift != 0 && shift != 16 && shift != 32 && shift != 48) {
        throw std::invalid_argument("Invalid shift for MOVZ. Must be 0, 16, 32, or 48.");
    }

    // Helper lambda to parse the destination register.
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

    auto [rd_num, rd_is_64] = parse_register(xd);

    // A 32-bit register cannot be shifted by 32 or 48.
    if (!rd_is_64 && (shift == 32 || shift == 48)) {
        throw std::invalid_argument("Cannot use a shift of 32 or 48 with a 32-bit 'W' register.");
    }

    // (B) Use BitPatcher to construct the instruction word.
    // Base opcode for MOVZ is 0x52800000.
    BitPatcher patcher(0x52800000);

    // Set the size flag (sf) for a 64-bit register destination.
    if (rd_is_64) {
        patcher.patch(1, 31, 1);
    }

    // Encode the shift into the 'hw' field (bits 21-22).
    uint32_t hw = shift / 16;
    patcher.patch(hw, 21, 2);

    // Patch the 16-bit immediate into bits 5-20.
    patcher.patch(immediate, 5, 16);

    // Patch the destination register (Rd) into bits 0-4.
    patcher.patch(rd_num, 0, 5);

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "MOVZ " + xd + ", #" + std::to_string(immediate);
    if (shift > 0) {
        assembly_text += ", LSL #" + std::to_string(shift);
    }

    // (D) Return the completed Instruction object, including relocation info.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::MOVZ;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    return instr;
}
