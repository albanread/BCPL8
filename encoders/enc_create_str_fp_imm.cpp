#include "../Encoder.h"
#include <stdexcept>
#include "BitPatcher.h"

/**
 * @brief Encodes the ARM64 'STR' (Store Register) for a floating-point/SIMD register.
 * @details
 * This function generates the machine code to store a 64-bit floating-point
 * register (Dd) to memory using a base register (Xn) and an unsigned,
 * scaled immediate offset. The operation is `STR <Dt>, [<Xn>{, #imm}]`.
 *
 * The encoding follows the "Load/Store Register (unsigned immediate)" format:
 * - **size (bits 31-30)**: `11` for 64-bit.
 * - **V (bit 26)**: `1` for SIMD/FP.
 * - **opc (bits 23-22)**: `00` for STR.
 * - **imm12 (bits 21-10)**: A 12-bit unsigned immediate, scaled by 8 for 64-bit access.
 * - **Rn (bits 9-5)**: The base address register.
 * - **Rt (bits 4-0)**: The source floating-point register.
 *
 * @param dt The source floating-point register (e.g., "D0", "D7").
 * @param xn The base address register (e.g., "X29", "SP").
 * @param immediate The unsigned byte offset, which must be a multiple of 8 in the range [0, 32760].
 * @return An `Instruction` object.
 */
Instruction Encoder::create_str_fp_imm(const std::string& dt, const std::string& xn, int immediate) {
    // Validate that the offset is within the valid range and a multiple of 8.
    if (immediate < 0 || immediate > 32760 || immediate % 8 != 0) {
        throw std::runtime_error("STR (FP) immediate offset is out of range [0, 32760] or not a multiple of 8.");
    }

    uint32_t dt_reg = Encoder::get_reg_encoding(dt);
    uint32_t xn_reg = Encoder::get_reg_encoding(xn);
    uint32_t imm12 = static_cast<uint32_t>(immediate / 8);

    // The base opcode for STR Dt, [Xn, #imm] is 0xFD000000.
    // This is derived from the LDR equivalent (0xFD400000) by clearing bit 22 (the L bit).
    BitPatcher patcher(0xFD000000);

    // Patch the required fields into the base opcode.
    patcher.patch(imm12, 10, 12); // Patch the 12-bit scaled immediate.
    patcher.patch(xn_reg, 5, 5);  // Patch the base register (Rn).
    patcher.patch(dt_reg, 0, 5);  // Patch the source register (Rt).

    // Format the assembly string for clarity in listings and debugging.
    std::string assembly_text = "STR " + dt + ", [" + xn + ", #" + std::to_string(immediate) + "]";

    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::STR_FP;
    instr.src_reg1 = Encoder::get_reg_encoding(dt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
