#include "BitPatcher.h"
#include "Encoder.h"
#include <stdexcept>

Instruction Encoder::create_ldp_pre_imm(const std::string& xt1, const std::string& xt2, const std::string& xn, int immediate) {
    uint32_t rt1_val = get_reg_encoding(xt1);
    uint32_t rt2_val = get_reg_encoding(xt2);
    uint32_t rn_val = get_reg_encoding(xn);

    if (immediate % 8 != 0) {
        throw std::invalid_argument("LDP immediate must be a multiple of 8.");
    }

    int imm7 = immediate / 8;
    if (imm7 < -64 || imm7 > 63) {
        throw std::out_of_range("LDP immediate out of range for 7-bit signed offset.");
    }

    BitPatcher patcher(0xA9C00000);
    patcher.patch(imm7, 15, 7);
    patcher.patch(rt2_val, 10, 5);
    patcher.patch(rn_val, 5, 5);
    patcher.patch(rt1_val, 0, 5);

    std::string assembly = "LDP " + xt1 + ", " + xt2 + ", [" + xn + ", #" + std::to_string(immediate) + "]!";
    Instruction instr(patcher.get_value(), assembly);
    instr.opcode = InstructionDecoder::OpType::LDP;
    instr.dest_reg = Encoder::get_reg_encoding(xt1);
    instr.src_reg1 = Encoder::get_reg_encoding(xt2);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}
