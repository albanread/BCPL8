#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <memory>
#include <iostream>
#include <algorithm>
#include <regex>
#include <cmath>

/**
 * @brief Creates a pattern to eliminate identity operations.
 * 
 * This pattern recognizes and eliminates instructions that have no effect on program state,
 * such as adding/subtracting zero, multiplying/dividing by one, or subtracting a register 
 * from itself. These inefficient operations often occur as byproducts of the code generation process.
 * 
 * Patterns detected:
 * 1. ADD/SUB with zero: ADD Xd, Xs, #0 -> MOV Xd, Xs
 * 2. MUL/DIV by one: MUL Xd, Xs, #1 -> MOV Xd, Xs
 * 3. Self-subtraction: SUB Xd, Xs, Xs -> MOV Xd, #0
 * 
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createIdentityOperationEliminationPattern() {
    return std::make_unique<InstructionPattern>(
        1, // Pattern size of 1 instruction
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            if (pos >= instrs.size()) return false;
            const auto& instr = instrs[pos];
            auto opcode = instr.opcode;

            // Case 1: ADD/SUB with zero
            if (opcode == InstructionDecoder::OpType::ADD || opcode == InstructionDecoder::OpType::SUB) {
                if (instr.uses_immediate && instr.immediate == 0) {
                    return true;
                }
                // Check if it uses XZR/WZR as second operand (register number 31)
                if (instr.src_reg2 == 31) {
                    return true;
                }
            }

            // Case 2: MUL/DIV by one
            if (opcode == InstructionDecoder::OpType::MUL || opcode == InstructionDecoder::OpType::DIV) {
                if (instr.uses_immediate && std::abs(instr.immediate) == 1) {
                    return true;
                }
            }

            // Case 3: Self-subtraction (SUB Xd, Xs, Xs)
            if (opcode == InstructionDecoder::OpType::SUB) {
                if (instr.src_reg1 != -1 && instr.src_reg2 != -1 &&
                    InstructionComparator::areSameRegister(instr.src_reg1, instr.src_reg2)) {
                    return true;
                }
            }

            return false;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr = instrs[pos];
            auto opcode = instr.opcode;

            // Case 1: ADD/SUB with zero or XZR/WZR
            if ((opcode == InstructionDecoder::OpType::ADD || opcode == InstructionDecoder::OpType::SUB) &&
                ((instr.uses_immediate && instr.immediate == 0) || instr.src_reg2 == 31)) {
                // Replace with MOV dest, src
                return { Encoder::create_mov_reg(instr.dest_reg, instr.src_reg1) };
            }

            // Case 2: MUL/DIV by one or minus one
            if ((opcode == InstructionDecoder::OpType::MUL || opcode == InstructionDecoder::OpType::DIV) &&
                instr.uses_immediate && std::abs(instr.immediate) == 1) {
                if ((opcode == InstructionDecoder::OpType::DIV && instr.immediate == 1) ||
                    (opcode == InstructionDecoder::OpType::MUL && instr.immediate == 1)) {
                    return { Encoder::create_mov_reg(instr.dest_reg, instr.src_reg1) };
                } else if (opcode == InstructionDecoder::OpType::MUL && instr.immediate == -1) {
                    // Use NEG (which is SUB dest, XZR, src)
                    return { Encoder::create_neg(instr.dest_reg, instr.src_reg1) };
                }
            }

            // Case 3: Self-subtraction (SUB Xd, Xs, Xs)
            if (opcode == InstructionDecoder::OpType::SUB &&
                instr.src_reg1 != -1 && instr.src_reg2 != -1 &&
                InstructionComparator::areSameRegister(instr.src_reg1, instr.src_reg2)) {
                // Replace with MOVZ dest, #0
                return { Encoder::create_movz_imm(instr.dest_reg, 0) };
            }

            // If we get here, no pattern matched
            return { instr };
        },
        "Identity Operation Elimination"
    );
}