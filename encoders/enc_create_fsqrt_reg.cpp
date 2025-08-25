#include "Encoder.h"
#include "BitPatcher.h"

// Implements the ARM64 FSQRT Dd, Dn instruction encoding.
// FSQRT (double-precision) has base encoding 0x1E61C000.
// Dd: destination FP register (0-31), Dn: source FP register (0-31).
Instruction Encoder::create_fsqrt_reg(const std::string& dd, const std::string& dn) {
    uint32_t rd = get_reg_encoding(dd);
    uint32_t rn = get_reg_encoding(dn);

    // Encoding for FSQRT Dd, Dn (double-precision): 0x1E61C000
    BitPatcher patcher(0x1E61C000);
    patcher.patch(rn, 5, 5);  // Dn at bits [9:5]
    patcher.patch(rd, 0, 5);  // Dd at bits [4:0]

    std::string asm_text = "FSQRT " + dd + ", " + dn;
    Instruction instr(patcher.get_value(), asm_text);
    instr.opcode = InstructionDecoder::OpType::FSQRT;
    instr.dest_reg = rd + 32; // FP register encoding offset
    instr.src_reg1 = rn + 32; // FP register encoding offset
    return instr;
}