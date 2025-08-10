#include "InstructionComparator.h"
#include "InstructionDecoder.h"

using namespace InstructionDecoder;

bool InstructionComparator::haveSameOpcode(const Instruction& instr1, const Instruction& instr2) {
    return getOpcode(instr1) == getOpcode(instr2);
}

bool InstructionComparator::haveSameRegisters(const Instruction& instr1, const Instruction& instr2) {
    return areSameRegister(getDestReg(instr1), getDestReg(instr2)) &&
           areSameRegister(getSrcReg1(instr1), getSrcReg1(instr2)) &&
           areSameRegister(getSrcReg2(instr1), getSrcReg2(instr2));
}

bool InstructionComparator::haveSameImmediate(const Instruction& instr1, const Instruction& instr2) {
    if (usesImmediate(instr1) && usesImmediate(instr2)) {
        return getImmediate(instr1) == getImmediate(instr2);
    }
    return !usesImmediate(instr1) && !usesImmediate(instr2);
}

bool InstructionComparator::haveSameMemoryOperand(const Instruction& instr1, const Instruction& instr2) {
    if (isMemoryOp(instr1) && isMemoryOp(instr2)) {
        return getBaseReg(instr1) == getBaseReg(instr2) &&
               getOffset(instr1) == getOffset(instr2);
    }
    return !isMemoryOp(instr1) && !isMemoryOp(instr2);
}

bool InstructionComparator::areSemanticallyEqual(const Instruction& instr1, const Instruction& instr2) {
    if (!haveSameOpcode(instr1, instr2) ||
        !haveSameRegisters(instr1, instr2) ||
        !haveSameImmediate(instr1, instr2) ||
        !haveSameMemoryOperand(instr1, instr2)) {
        return false;
    }
    return true;
}

bool InstructionComparator::areSameRegister(int reg1_num, int reg2_num) {
    if (reg1_num < 0 || reg2_num < 0) {
        return false; // -1 indicates an unused register operand.
    }
    // W registers (32-bit) and X registers (64-bit) share the same number ID.
    return (reg1_num % 32) == (reg2_num % 32);
}