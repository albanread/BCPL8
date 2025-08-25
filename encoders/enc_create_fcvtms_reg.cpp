#include "Encoder.h"
#include <sstream>
#include "BitPatcher.h"

Instruction Encoder::create_fcvtms_reg(const std::string& xd, const std::string& dn) {
    uint32_t rd = get_reg_encoding(xd);
    uint32_t rn = get_reg_encoding(dn);

    // Encoding for FCVTMS Xd, Dn (from double to signed 64-bit, rounding toward minus infinity)
    BitPatcher patcher(0x9E780000);
    patcher.patch(rn, 5, 5); // Dn at bits [9:5]
    patcher.patch(rd, 0, 5);  // Xd at bits [4:0]

    std::stringstream ss;
    ss << "FCVTMS " << xd << ", " << dn;
    Instruction instr(patcher.get_value(), ss.str());
    instr.opcode = InstructionDecoder::OpType::FCVTMS; // You may need to add this OpType
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(dn);
    return instr;
}
