#include "Encoder.h"
#include "BitPatcher.h" // Make sure to include BitPatcher
#include <sstream>

Instruction Encoder::create_ldr_fp_imm(const std::string& dt, const std::string& xn, int immediate) {
    // Unsigned offset, must be a multiple of the transfer size (8 bytes for a double).
    if (immediate < 0 || immediate > 32760 || immediate % 8 != 0) {
        throw std::runtime_error("LDR (FP) immediate offset out of range [0, 32760] or not a multiple of 8.");
    }

    uint32_t rt = get_reg_encoding(dt);
    uint32_t rn = get_reg_encoding(xn);
    uint32_t imm12 = static_cast<uint32_t>(immediate / 8);

    // CORRECTED: Use the base opcode for a 64-bit floating-point load (0xFD400000).
    // The previous opcode (0xBD400000) was for a 32-bit load.
    BitPatcher patcher(0xFD400000);

    // Patch the required fields into the base opcode.
    patcher.patch(imm12, 10, 12); // Patch the 12-bit scaled immediate.
    patcher.patch(rn, 5, 5);      // Patch the base register (Rn).
    patcher.patch(rt, 0, 5);      // Patch the destination register (Rt).

    std::stringstream ss;
    ss << "LDR " << dt << ", [" << xn << ", #" << immediate << "]";
    Instruction instr(patcher.get_value(), ss.str());
    instr.opcode = InstructionDecoder::OpType::LDR_FP;
    instr.dest_reg = Encoder::get_reg_encoding(dt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
