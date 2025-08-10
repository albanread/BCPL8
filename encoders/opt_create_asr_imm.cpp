#include <algorithm>
#include <cstdint>
#include "BitPatcher.h"
#include "Encoder.h"
#include <string>
#include <stdexcept>

/**
 * @brief Encodes the ARM64 'ASR' (Arithmetic Shift Right) instruction with an immediate.
 * @details
 * This function generates the machine code to perform an arithmetic right shift
 * on a register by an immediate amount. `ASR` is an alias for the `SBFM`
 * (Signed Bitfield Move) instruction.
 * The operation is `ASR <Xd|Wd>, <Xn|Wn>, #<shift>`.
 *
 * The encoding follows the "Bitfield" format for SBFM:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-29)**: `00` for SBFM.
 * - **Family (bits 28-23)**: `0b100100`.
 * - **N (bit 22)**: Must match `sf`.
 * - **immr (bits 21-16)**: The shift amount.
 * - **imms (bits 15-10)**: The destination bitfield width, which is `datasize - 1`.
 * - **Rn (bits 9-5)**: The source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0", "w1").
 * @param xn The source register to be shifted.
 * @param shift_amount The immediate shift amount (0-63 for X regs, 0-31 for W regs).
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or out-of-range shift amount.
 */
Instruction Encoder::opt_create_asr_imm(const std::string& xd, const std::string& xn, int shift_amount) {
    // 1. Validate register names and determine size
    uint32_t rd_num = get_reg_encoding(xd);
    uint32_t rn_num = get_reg_encoding(xn);
    bool is_64bit = (xd[0] == 'x' || xd[0] == 'X');

    if (is_64bit != (xn[0] == 'x' || xn[0] == 'X')) {
        throw std::invalid_argument("Mismatched register sizes for ASR (immediate).");
    }

    // 2. Validate shift amount
    int datasize = is_64bit ? 64 : 32;
    if (shift_amount < 0 || shift_amount >= datasize) {
        throw std::invalid_argument("ASR shift amount is out of range for the register size.");
    }

    // 3. Calculate encoding fields for the SBFM alias
    uint32_t n_val = is_64bit ? 1 : 0;
    uint32_t immr_val = static_cast<uint32_t>(shift_amount);
    uint32_t imms_val = static_cast<uint32_t>(datasize - 1);

    // 4. Use BitPatcher to construct the instruction word.
    // Base opcode for SBFM is 0x13000000.
    BitPatcher patcher(0x13000000);

    if (is_64bit) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(n_val, 22, 1);       // N bit
    patcher.patch(immr_val, 16, 6);    // immr (shift amount)
    patcher.patch(imms_val, 10, 6);    // imms (datasize - 1)
    patcher.patch(rn_num, 5, 5);       // Rn
    patcher.patch(rd_num, 0, 5);       // Rd

    // 5. Format the assembly string and return the Instruction object.
    std::string assembly_text = "ASR " + xd + ", " + xn + ", #" + std::to_string(shift_amount);
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::ASR;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.immediate = shift_amount;
    instr.uses_immediate = true;
    return instr;
}