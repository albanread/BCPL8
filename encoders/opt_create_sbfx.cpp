#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "BitPatcher.h"
#include "Encoder.h"
/**
 * @brief Encodes the ARM64 'SBFX' (Signed Bitfield Extract) instruction.
 * @details
 * This function generates the 32-bit machine code to extract a bitfield from a
 * source register, sign-extend it, and place it in a destination register.
 * SBFX is an alias for the SBFM (Signed Bitfield Move) instruction.
 * The operation is `SBFX <Xd|Wd>, <Xn|Wn>, #lsb, #width`.
 *
 * The encoding follows the "Bitfield" format for SBFM:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-29)**: `00` for SBFM.
 * - **Family (bits 28-23)**: `0b100100`.
 * - **N (bit 22)**: Must match `sf`.
 * - **immr (bits 21-16)**: The least significant bit of the source field (`lsb`).
 * - **imms (bits 15-10)**: The most significant bit of the source field (`lsb + width - 1`).
 * - **Rn (bits 9-5)**: The source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0", "w1").
 * @param xn The source register (e.g., "x1", "w2").
 * @param lsb The least significant bit of the field to extract (0-63).
 * @param width The width of the bitfield (1-64).
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or bitfield parameters.
 */
Instruction Encoder::opt_create_sbfx(const std::string& xd, const std::string& xn, int lsb, int width) {
    // 1. Validate register names and determine size
    uint32_t rd_num = get_reg_encoding(xd);
    uint32_t rn_num = get_reg_encoding(xn);
    bool is_64bit = (xd[0] == 'x' || xd[0] == 'X');

    if (is_64bit != (xn[0] == 'x' || xn[0] == 'X')) {
        throw std::invalid_argument("Mismatched register sizes for SBFX.");
    }

    // 2. Validate bitfield parameters
    int datasize = is_64bit ? 64 : 32;
    if (lsb < 0 || lsb >= datasize) {
        throw std::invalid_argument("SBFX lsb is out of range for the register size.");
    }
    if (width < 1 || (lsb + width) > datasize) {
        throw std::invalid_argument("SBFX width is out of range or exceeds register size.");
    }

    // 3. Calculate encoding fields for the SBFM alias
    uint32_t n_val = is_64bit ? 1 : 0;
    uint32_t immr_val = static_cast<uint32_t>(lsb);
    uint32_t imms_val = static_cast<uint32_t>(lsb + width - 1);

    // 4. Use BitPatcher to construct the instruction word.
    // Base opcode for SBFM is 0x13000000.
    BitPatcher patcher(0x13000000);

    if (is_64bit) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(n_val, 22, 1);       // N bit
    patcher.patch(immr_val, 16, 6);    // immr (lsb)
    patcher.patch(imms_val, 10, 6);    // imms (msb)
    patcher.patch(rn_num, 5, 5);       // Rn
    patcher.patch(rd_num, 0, 5);       // Rd

    // 5. Format the assembly string and return the Instruction object.
    std::string assembly_text = "SBFX " + xd + ", " + xn + ", #" + std::to_string(lsb) + ", #" + std::to_string(width);
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::SBFX;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.immediate = lsb;
    instr.uses_immediate = true;
    return instr;
}