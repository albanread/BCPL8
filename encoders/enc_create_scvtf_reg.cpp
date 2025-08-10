#include "Encoder.h"
#include <sstream>
#include "BitPatcher.h"

Instruction Encoder::create_scvtf_reg(const std::string& dd, const std::string& xn) {
    uint32_t rd = get_reg_encoding(dd);
    uint32_t rn = get_reg_encoding(xn);

    // Use BitPatcher for encoding SCVTF Dd, Xn (from signed 64-bit int to double)
    BitPatcher patcher(0x9E620000);
    patcher.patch(rn, 5, 5); // Patch source register (Rn)
    patcher.patch(rd, 0, 5); // Patch destination register (Rd)

    std::stringstream ss;
    ss << "SCVTF " << dd << ", " << xn;
    Instruction instr(patcher.get_value(), ss.str());
    instr.opcode = InstructionDecoder::OpType::SCVTF;
    instr.dest_reg = Encoder::get_reg_encoding(dd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    return instr;
}
