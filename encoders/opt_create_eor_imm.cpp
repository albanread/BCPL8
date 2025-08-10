#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "BitPatcher.h"
#include "Encoder.h"
/**
 * @brief Encodes the ARM64 'EOR' (Bitwise Exclusive OR) instruction with an immediate.
 * @details
 * This function generates the 32-bit machine code for an EOR instruction
 * that performs a bitwise XOR between a register and a bitmask immediate.
 * The instruction has the format: `EOR <Xd|Wd>, <Xn|Wn>, #imm`.
 *
 * The encoding follows the "Logical (immediate)" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-29)**: `10` for EOR.
 * - **Family (bits 28-23)**: `0b100100`.
 * - **N (bit 22)**: Encoded based on the immediate pattern.
 * - **immr (bits 21-16)**: Encoded rotation for the immediate.
 * - **imms (bits 15-10)**: Encoded size/pattern for the immediate.
 * - **Rn (bits 9-5)**: The source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0", "w1").
 * @param xn The source register (e.g., "x1", "sp").
 * @param immediate The immediate value, which must be encodable as a bitmask.
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or unencodable immediates.
 */
Instruction Encoder::opt_create_eor_imm(const std::string& xd, const std::string& xn, int64_t immediate) {
    // 1. Validate register names and determine size
    uint32_t rd_num = get_reg_encoding(xd);
    uint32_t rn_num = get_reg_encoding(xn);
    bool is_64bit = (xd[0] == 'x' || xd[0] == 'X');

    // Ensure register sizes match (simplified check)
    if (is_64bit != (xn[0] == 'x' || xn[0] == 'X')) {
         if ((xn != "sp" && xn != "SP") && (xd != "sp" && xd != "SP")) {
            throw std::invalid_argument("Mismatched register sizes for EOR (immediate).");
         }
    }

    // 2. Encode the bitmask immediate
    uint32_t n_val, immr_val, imms_val;
    if (!encode_bitmask_immediate(static_cast<uint64_t>(immediate), is_64bit, n_val, immr_val, imms_val)) {
        throw std::invalid_argument("Immediate value " + std::to_string(immediate) + " cannot be encoded for EOR instruction.");
    }

    // 3. Use BitPatcher to construct the instruction word.
    // Base opcode for Logical (immediate) with opc=10 (EOR) is 0x52000000.
    BitPatcher patcher(0x52000000);

    if (is_64bit) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(n_val, 22, 1);       // N bit
    patcher.patch(immr_val, 16, 6);    // immr
    patcher.patch(imms_val, 10, 6);    // imms
    patcher.patch(rn_num, 5, 5);       // Rn
    patcher.patch(rd_num, 0, 5);       // Rd

    // 4. Format the assembly string and return the Instruction object.
    std::string assembly_text = "EOR " + xd + ", " + xn + ", #" + std::to_string(immediate);
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::EOR;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    return instr;
}