#include <cstdint>
#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'ASR (register)' instruction (ASRV).
 * @details
 * This function generates the 32-bit machine code for an ASR instruction
 * that performs an arithmetic right shift on a register by a variable amount
 * specified in a second register. The canonical name for this instruction is ASRV.
 * The instruction has the format: `ASR <Xd|Wd>, <Xn|Wn>, <Xm|Wm>`.
 *
 * The encoding follows the "Data-processing (2 source)" format for ASRV:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **S (bit 30)**: Must be 0.
 * - **Family (bits 29-22)**: Fixed value of `0b11010110`.
 * - **Rm (bits 20-16)**: The register containing the shift amount (`xm`).
 * - **opcode2 (bits 15-10)**: Fixed value of `0b000100` for ASRV.
 * - **Rn (bits 9-5)**: The source register to be shifted (`xn`).
 * - **Rd (bits 4-0)**: The destination register (`xd`).
 *
 * @param xd The destination register (e.g., "x0").
 * @param xn The source register to be shifted.
 * @param xm The register containing the shift amount.
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or mismatched sizes.
 */
Instruction Encoder::opt_create_asr_reg(const std::string& xd, const std::string& xn, const std::string& xm) {
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
    auto [rm_num, rm_is_64] = parse_register(xm);

    if (!(rd_is_64 == rn_is_64 && rn_is_64 == rm_is_64)) {
        throw std::invalid_argument("Mismatched register sizes. All operands for ASR (register) must be the same size.");
    }

    // (B) Use BitPatcher to construct the instruction word.
    // Base opcode for 32-bit ASRV is 0x1AC01000.
    BitPatcher patcher(0x1AC01000);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    // Patch the registers into their respective fields.
    patcher.patch(rd_num, 0, 5);  // Rd
    patcher.patch(rn_num, 5, 5);  // Rn
    patcher.patch(rm_num, 16, 5); // Rm

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "ASR " + xd + ", " + xn + ", " + xm;

    // (D) Return the completed Instruction object. No relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::ASR;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    return instr;
}