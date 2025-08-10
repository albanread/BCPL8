#include "Encoder.h"
#include <sstream>

Instruction Encoder::create_svc_imm(uint16_t immediate) {
     // SVC #imm (Supervisor Call)
     // Encoding: 11010100 00000000 00000000 iiiiiiii (imm16 in bits 20-5)
     uint32_t encoding = 0xD4000001 | ((static_cast<uint32_t>(immediate) & 0xFFFF) << 5);
     std::stringstream ss;
     ss << "SVC #" << immediate;
     Instruction instr(encoding, ss.str());
    instr.opcode = InstructionDecoder::OpType::SVC;
    instr.immediate = immediate;
    instr.uses_immediate = true;
    return instr;
 }
