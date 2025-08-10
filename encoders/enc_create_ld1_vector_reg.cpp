#include "Encoder.h"
#include <sstream>

Instruction Encoder::create_ld1_vector_reg(const std::string& vt, const std::string& xn, const std::string& arrangement) {
    uint32_t rt = get_reg_encoding(vt);
    uint32_t rn = get_reg_encoding(xn);

    // Encoding for LD1 {Vt.4S}, [Xn]
    // Q(1) | 0 | 0 | 0110 | 1 | L(1) | M(0) | 1 | Rm(11111) | 1100 | Rn | Rt
    uint32_t encoding = 0x4C40CC00 | (rn << 5) | rt;

    std::stringstream ss;
    ss << "LD1 {" << vt << "." << arrangement << "}, [" << xn << "]";
    Instruction instr(encoding, ss.str());
    instr.opcode = InstructionDecoder::OpType::LD1_VECTOR;
    instr.dest_reg = Encoder::get_reg_encoding(vt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.is_mem_op = true;
    return instr;
}
