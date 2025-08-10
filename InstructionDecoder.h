#ifndef INSTRUCTION_DECODER_H
#define INSTRUCTION_DECODER_H

#include <cstdint>
#include <string>
#include "OpType.h"
#include "Encoder.h"

namespace InstructionDecoder {

// NOTE: The OpType enum itself lives in OpType.h
// This file only contains the accessor functions.

inline OpType getOpcode(const Instruction& instr) { return instr.opcode; }
inline int getDestReg(const Instruction& instr) { return instr.dest_reg; }
inline int getSrcReg1(const Instruction& instr) { return instr.src_reg1; }
inline int getSrcReg2(const Instruction& instr) { return instr.src_reg2; }
inline int getBaseReg(const Instruction& instr) { return instr.base_reg; }
inline int getRaReg(const Instruction& instr) { return instr.ra_reg; }
// Missing functions added below
inline int getRn(const Instruction& instr) { return instr.src_reg1; } // Rn is an alias for the first source register
inline int getRm(const Instruction& instr) { return instr.src_reg2; } // Rm is an alias for the second source register
inline int64_t getOffset(const Instruction& instr) { return instr.immediate; } // For memory ops, offset is stored in immediate

inline int64_t getImmediate(const Instruction& instr) { return instr.immediate; }
inline bool usesImmediate(const Instruction& instr) { return instr.uses_immediate; }
inline bool isMemoryOp(const Instruction& instr) { return instr.is_mem_op; }

// Corrected return type from "Instruction::JITAttribute" to just "JITAttribute"
inline JITAttribute getJITAttribute(const Instruction& instr) { return instr.jit_attribute; }

// Helper function declaration
std::string getRegisterName(int reg_num);

// Declarations for the functions your other files need
std::string getDestRegAsString(const Instruction& instr);
std::string getSrcReg1AsString(const Instruction& instr);
std::string getSrcReg2AsString(const Instruction& instr); // Good to add for consistency

} // namespace InstructionDecoder

#endif // INSTRUCTION_DECODER_H