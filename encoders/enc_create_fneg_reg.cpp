#include "Encoder.h"
#include <sstream>
#include "BitPatcher.h"

// Implements FNEG Dd, Dn (floating-point negate, double-precision)
Instruction Encoder::create_fneg_reg(const std::string& dd, const std::string& dn) {
    uint32_t rd = get_reg_encoding(dd);
    uint32_t rn = get_reg_encoding(dn);

    // Encoding for FNEG Dd, Dn:
    // 0x1E614000 | (rn << 5) | rd
    // FNEG Dd, Dn: 0001 1110 0110 0001 0100 0000 000r nnnn rrrrr
    BitPatcher patcher(0x1E614000);
    patcher.patch(rn, 5, 5); // Dn at bits [9:5]
    patcher.patch(rd, 0, 5); // Dd at bits [4:0]

    std::stringstream ss;
    ss << "FNEG " << dd << ", " << dn;
    Instruction instr(patcher.get_value(), ss.str());
    instr.opcode = InstructionDecoder::OpType::FMOV; // No dedicated FNEG OpType, FMOV is often used for unary float ops
    instr.dest_reg = rd;
    instr.src_reg1 = rn;
    return instr;
}