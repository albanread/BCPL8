#include "Encoder.h"
#include <sstream>

Instruction Encoder::create_add_vector_reg(const std::string& vd, const std::string& vn, const std::string& vm, const std::string& arrangement) {
    uint32_t rd = get_reg_encoding(vd);
    uint32_t rn = get_reg_encoding(vn);
    uint32_t rm = get_reg_encoding(vm);

    // Encoding for ADD Vd.4S, Vn.4S, Vm.4S (vector integer add)
    // Q(1) | 0 | 0 | 01110 | 1 | M(0) | 1 | Rm | 1000 | Rn | Rd
    uint32_t encoding = 0x4E208000 | (rm << 16) | (rn << 5) | rd;

    std::stringstream ss;
    ss << "ADD " << vd << "." << arrangement << ", " << vn << "." << arrangement << ", " << vm << "." << arrangement;
    Instruction instr(encoding, ss.str());
    instr.opcode = InstructionDecoder::OpType::ADD_VECTOR;
    instr.dest_reg = Encoder::get_reg_encoding(vd);
    instr.src_reg1 = Encoder::get_reg_encoding(vn);
    instr.src_reg2 = Encoder::get_reg_encoding(vm);
    return instr;
}
