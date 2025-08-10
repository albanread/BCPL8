#include "Encoder.h"
#include <sstream>

Instruction Encoder::create_fcmp_reg(const std::string& dn, const std::string& dm) {
    uint32_t rn = get_reg_encoding(dn);
    uint32_t rm = get_reg_encoding(dm);

    // Encoding for FCMP Dn, Dm (double-precision)
    uint32_t encoding = 0x1E602008 | (rm << 16) | (rn << 5);

    std::stringstream ss;
    ss << "FCMP " << dn << ", " << dm;
    Instruction instr(encoding, ss.str());
    instr.opcode = InstructionDecoder::OpType::FCMP;
    instr.src_reg1 = Encoder::get_reg_encoding(dn);
    instr.src_reg2 = Encoder::get_reg_encoding(dm);
    return instr;
}
