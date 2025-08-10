#ifndef INSTRUCTION_COMPARATOR_H
#define INSTRUCTION_COMPARATOR_H

#include "Encoder.h"
#include <vector>

/**
 * @brief Utility class for comparing Instruction objects.
 */
class InstructionComparator {
public:
    /**
     * @brief Checks if two instructions have the same operation type.
     */
    static bool haveSameOpcode(const Instruction& instr1, const Instruction& instr2);

    /**
     * @brief Checks if two instructions use the exact same register operands
     * (destination, source 1, source 2).
     */
    static bool haveSameRegisters(const Instruction& instr1, const Instruction& instr2);

    /**
     * @brief Checks if two instructions that use immediate values have the same immediate.
     * Returns true if neither uses an immediate; false if only one does.
     */
    static bool haveSameImmediate(const Instruction& instr1, const Instruction& instr2);

    /**
     * @brief Checks if two memory-access instructions use the same base register and offset.
     * Returns true if neither is a memory op; false if only one is.
     */
    static bool haveSameMemoryOperand(const Instruction& instr1, const Instruction& instr2);

    /**
     * @brief Performs a comprehensive check to see if two instructions are semantically identical
     * (same opcode, registers, immediate, and memory operands).
     */
    static bool areSemanticallyEqual(const Instruction& instr1, const Instruction& instr2);

    /**
     * @brief Checks if two register numbers refer to the same physical register,
     * accounting for W/X register aliasing (e.g., W0 and X0 are the same).
     * Returns false if either register number is negative (unused).
     */
    static bool areSameRegister(int reg1_num, int reg2_num);
};

#endif // INSTRUCTION_COMPARATOR_H