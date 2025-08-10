#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "BitPatcher.h"
#include "Encoder.h"
/**
 * @brief Encodes the ARM64 'CSINV' (Conditional Select Invert) instruction.
 * @details
 * This function generates the 32-bit machine code to conditionally invert a
 * register's value. If the condition is true, the destination register is set
 * to the value of the first source register (Rn). If the condition is false,
 * it is set to the bitwise inverse of the second source register (Rm).
 * The operation is `CSINV <Xd|Wd>, <Xn|Wn>, <Xm|Wm>, <cond>`.
 *
 * The encoding follows the "Conditional select" format:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **op (bit 30)**: `1`.
 * - **S (bit 29)**: `0`.
 * - **Family (bits 28-24)**: `0b11010`.
 * - **op2 (bits 11-10)**: `0b00`.
 * - **Rm (bits 20-16)**: The second source register `xm`.
 * - **cond (bits 15-12)**: The 4-bit condition code.
 * - **Rn (bits 9-5)**: The first source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param rd The destination register (e.g., "x0", "w1").
 * @param rn The first source register (used when condition is true).
 * @param rm The second source register (inverted when condition is false).
 * @param cond The condition mnemonic as a string (e.g., "EQ", "NE", "LT").
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers, mismatched sizes, or unrecognized conditions.
 */
Instruction Encoder::opt_create_csinv(const std::string& rd, const std::string& rn, const std::string& rm, const std::string& cond) {
    // 1. Validate register names and determine size
    uint32_t rd_num = get_reg_encoding(rd);
    uint32_t rn_num = get_reg_encoding(rn);
    uint32_t rm_num = get_reg_encoding(rm);
    bool is_64bit = (rd[0] == 'x' || rd[0] == 'X');

    // Ensure all registers are of the same size
    if (is_64bit != (rn[0] == 'x' || rn[0] == 'X') || is_64bit != (rm[0] == 'x' || rm[0] == 'X')) {
        // Allow for zero registers (wzr/xzr) to be mixed
        if (!(rn == "wzr" || rn == "xzr" || rm == "wzr" || rm == "xzr")) {
            throw std::invalid_argument("Mismatched register sizes for CSINV.");
        }
    }

    // 2. Get the 4-bit encoding for the condition string.
    uint32_t cond_code = get_condition_code(cond);

    // 3. Use BitPatcher to construct the instruction word.
    // Base opcode for 32-bit CSINV is 0x5A800000.
    BitPatcher patcher(0x5A800000);

    if (is_64bit) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(rm_num, 16, 5);      // Rm
    patcher.patch(cond_code, 12, 4);   // cond
    patcher.patch(rn_num, 5, 5);       // Rn
    patcher.patch(rd_num, 0, 5);       // Rd

    // 4. Format the assembly string and return the Instruction object.
    std::string assembly_text = "CSINV " + rd + ", " + rn + ", " + rm + ", " + cond;
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::CSINV;
    instr.dest_reg = Encoder::get_reg_encoding(rd);
    instr.src_reg1 = Encoder::get_reg_encoding(rn);
    instr.src_reg2 = Encoder::get_reg_encoding(rm);
    return instr;
}