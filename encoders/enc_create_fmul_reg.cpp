#include "Encoder.h"
#include "BitPatcher.h" // Make sure to include BitPatcher
#include <sstream>

/**
 * @brief Encodes the ARM64 'FMUL' (Floating-point Multiply) instruction.
 * @details
 * This function generates the machine code to multiply two double-precision
 * floating-point registers and store the result in a third register.
 * The operation is `FMUL <Dd>, <Dn>, <Dm>`.
 *
 * The encoding follows the "Floating-point data-processing (2 source)" format:
 * - **M (bit 31)**: `0`
 * - **S (bit 30)**: `0`
 * - **type (bits 23-22)**: `01` for double-precision.
 * - **opcode (bits 20-12)**: `0001000` for FMUL.
 * - **Rm (bits 20-16)**: The second source register.
 * - **Rn (bits 9-5)**: The first source register.
 * - **Rd (bits 4-0)**: The destination register.
 *
 * @param dd The destination register (e.g., "D0").
 * @param dn The first source register (e.g., "D1").
 * @param dm The second source register (e.g., "D2").
 * @return An `Instruction` object.
 */
Instruction Encoder::create_fmul_reg(const std::string& dd, const std::string& dn, const std::string& dm) {
    uint32_t rd = get_reg_encoding(dd);
    uint32_t rn = get_reg_encoding(dn);
    uint32_t rm = get_reg_encoding(dm);

    // The correct base opcode for FMUL Dd, Dn, Dm (double-precision) is 0x1E600800.
    BitPatcher patcher(0x1E600800);

    // Patch the register fields into the base opcode.
    patcher.patch(rm, 16, 5); // Patch the second source register (Rm).
    patcher.patch(rn, 5, 5);  // Patch the first source register (Rn).
    patcher.patch(rd, 0, 5);  // Patch the destination register (Rd).

    std::stringstream ss;
    ss << "FMUL " << dd << ", " << dn << ", " << dm;
    Instruction instr(patcher.get_value(), ss.str());
    instr.opcode = InstructionDecoder::OpType::FMUL;
    instr.dest_reg = Encoder::get_reg_encoding(dd);
    instr.src_reg1 = Encoder::get_reg_encoding(dn);
    instr.src_reg2 = Encoder::get_reg_encoding(dm);
    return instr;
}
