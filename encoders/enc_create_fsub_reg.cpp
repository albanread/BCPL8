#include "Encoder.h"
#include "BitPatcher.h"
#include <sstream>

Instruction Encoder::create_fsub_reg(const std::string& dd, const std::string& dn, const std::string& dm) {
    uint32_t rd = get_reg_encoding(dd);
    uint32_t rn = get_reg_encoding(dn);
    uint32_t rm = get_reg_encoding(dm);

    // Correct encoding for FSUB Dd, Dn, Dm (double-precision, type=01)
    // Reference opcode: 0x1E203800
    BitPatcher patcher(0x1E203800);
    patcher.patch(rm, 16, 5); // Rm
    patcher.patch(rn, 5, 5);  // Rn
    patcher.patch(rd, 0, 5);  // Rd

    std::stringstream ss;
    ss << "FSUB " << dd << ", " << dn << ", " << dm;
    Instruction instr(patcher.get_value(), ss.str());
    instr.opcode = InstructionDecoder::OpType::FSUB;
    instr.dest_reg = Encoder::get_reg_encoding(dd);
    instr.src_reg1 = Encoder::get_reg_encoding(dn);
    instr.src_reg2 = Encoder::get_reg_encoding(dm);
    return instr;
}
