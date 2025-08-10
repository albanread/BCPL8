#include "Encoder.h"
#include <sstream>

Instruction Encoder::create_fcvtzs_reg(const std::string& xd, const std::string& dn) {
    uint32_t rd = get_reg_encoding(xd);
    uint32_t rn = get_reg_encoding(dn);

    // Encoding for FCVTZS Xd, Dn (from double to signed 64-bit, rounding to zero)
    uint32_t encoding = 0x9E630000 | (rn << 5) | rd;

    std::stringstream ss;
    ss << "FCVTZS " << xd << ", " << dn;
    Instruction instr(encoding, ss.str());
    instr.opcode = InstructionDecoder::OpType::FCVTZS;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(dn);
    return instr;
}
