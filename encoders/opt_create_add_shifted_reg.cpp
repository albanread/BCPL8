#include <cstdint>
#include "BitPatcher.h"
#include "Encoder.h"
#include <string>
#include <algorithm>
#include <stdexcept>

/**
 * @brief Encodes the ARM64 'ADD (shifted register)' instruction.
 * @details
 * This function generates the 32-bit machine code for an ADD instruction
 * that adds a register to a second register that has been shifted by an
 * immediate amount.
 * The instruction has the format: `ADD <Xd|Wd>, <Xn|Wn>, <Xm|Wm>{, <shift> #<amount>}`.
 *
 * The encoding follows the "Data-processing (register)" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-29)**: `0b00`.
 * - **S (bit 29)**: `0`.
 * - **Family (bits 28-25)**: `0b01011`.
 * - **shift (bits 23-22)**: `00` for LSL, `01` for LSR, `10` for ASR.
 * - **N (bit 21)**: `0`.
 * - **Rm (bits 20-16)**: The second source register `xm` (to be shifted).
 * - **imm6 (bits 15-10)**: The 6-bit shift amount.
 * - **Rn (bits 9-5)**: The first source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param rd The destination register (e.g., "x0", "w1").
 * @param rn The first source register.
 * @param rm The second source register (to be shifted).
 * @param shift_type The type of shift ("LSL", "LSR", or "ASR").
 * @param shift_amount The amount to shift by (0-63).
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or shift parameters.
 */
Instruction Encoder::opt_create_add_shifted_reg(const std::string& rd, const std::string& rn, const std::string& rm, const std::string& shift_type, int shift_amount) {
    // 1. Validate register names and determine size
    uint32_t rd_num = get_reg_encoding(rd);
    uint32_t rn_num = get_reg_encoding(rn);
    uint32_t rm_num = get_reg_encoding(rm);
    bool is_64bit = (rd[0] == 'x' || rd[0] == 'X');

    if (is_64bit != (rn[0] == 'x' || rn[0] == 'X') || is_64bit != (rm[0] == 'x' || rm[0] == 'X')) {
        throw std::invalid_argument("Mismatched register sizes for ADD (shifted register).");
    }

    // 2. Validate shift parameters
    int max_shift = is_64bit ? 63 : 31;
    if (shift_amount < 0 || shift_amount > max_shift) {
        throw std::invalid_argument("Shift amount is out of range for the register size.");
    }

    uint32_t shift_code;
    std::string upper_shift_type = shift_type;
    std::transform(upper_shift_type.begin(), upper_shift_type.end(), upper_shift_type.begin(), ::toupper);

    if (upper_shift_type == "LSL") {
        shift_code = 0b00;
    } else if (upper_shift_type == "LSR") {
        shift_code = 0b01;
    } else if (upper_shift_type == "ASR") {
        shift_code = 0b10;
    } else {
        throw std::invalid_argument("Invalid shift type for ADD (shifted register). Must be LSL, LSR, or ASR.");
    }

    // 3. Use BitPatcher to construct the instruction word.
    // Base opcode for ADD (shifted register) is 0x0B000000.
    BitPatcher patcher(0x0B000000);

    if (is_64bit) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(shift_code, 22, 2);              // shift
    patcher.patch(rm_num, 16, 5);                  // Rm
    patcher.patch(static_cast<uint32_t>(shift_amount), 10, 6); // imm6
    patcher.patch(rn_num, 5, 5);                   // Rn
    patcher.patch(rd_num, 0, 5);                   // Rd

    // 4. Format the assembly string and return the Instruction object.
    std::string assembly_text = "ADD " + rd + ", " + rn + ", " + rm + ", " + upper_shift_type + " #" + std::to_string(shift_amount);
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::ADD;
    instr.dest_reg = Encoder::get_reg_encoding(rd);
    instr.src_reg1 = Encoder::get_reg_encoding(rn);
    instr.src_reg2 = Encoder::get_reg_encoding(rm);
    instr.immediate = shift_amount;
    instr.uses_immediate = true;
    return instr;
}