#include "Encoder.h"
#include <sstream>

Instruction Encoder::create_fadd_reg(const std::string& dd, const std::string& dn, const std::string& dm) {
    uint32_t rd = get_reg_encoding(dd);
    uint32_t rn = get_reg_encoding(dn);
    uint32_t rm = get_reg_encoding(dm);

    // Encoding for FADD Dd, Dn, Dm (double-precision, type=01)
    uint32_t encoding = 0x1E602800 | (rm << 16) | (rn << 5) | rd;

    std::stringstream ss;
    ss << "FADD " << dd << ", " << dn << ", " << dm;
    Instruction instr(encoding, ss.str());
    instr.opcode = InstructionDecoder::OpType::FADD;
    instr.dest_reg = Encoder::get_reg_encoding(dd);
    instr.src_reg1 = Encoder::get_reg_encoding(dn);
    instr.src_reg2 = Encoder::get_reg_encoding(dm);
    return instr;
}
