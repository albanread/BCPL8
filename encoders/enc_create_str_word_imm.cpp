#include "Encoder.h"
#include "BitPatcher.h"

// Implements Encoder::create_str_word_imm for 32-bit STR (store word) instruction.
// STR Wt, [Xn, #imm12] -- stores a 32-bit word from Wt to [Xn + imm12]

Instruction Encoder::create_str_word_imm(const std::string& wt, const std::string& xn, int immediate) {
    // Base opcode for STR (word, unsigned immediate) is 0xB9000000
    BitPatcher patcher(0xB9000000);

    // Immediate is 12 bits, unsigned
    uint32_t imm12 = static_cast<uint32_t>(immediate) & 0xFFF;

    // Patch imm12 at bits [21:10]
    patcher.patch(imm12, 10, 12);

    // Patch Rn (base address register) at bits [9:5]
    patcher.patch(get_reg_encoding(xn), 5, 5);

    // Patch Rt (source register Wt) at bits [4:0]
    patcher.patch(get_reg_encoding(wt), 0, 5);

    std::string assembly_text = "STR " + wt + ", [" + xn + ", #" + std::to_string(immediate) + "]";
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::STR;
    instr.src_reg1 = Encoder::get_reg_encoding(wt);
    instr.base_reg = Encoder::get_reg_encoding(xn);
    instr.immediate = immediate;
    instr.uses_immediate = true;
    instr.is_mem_op = true;
    return instr;
}