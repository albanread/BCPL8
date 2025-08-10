#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <cmath>
#include <iostream>
#include <algorithm>

/**
 * @brief Creates a pattern to fuse consecutive ALU operations with immediates.
 * Pattern: ADD Xd, Xn, #imm1 ; ADD Xd, Xd, #imm2 → ADD Xd, Xn, #(imm1 + imm2)
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createFuseAluOperationsPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: 2 instructions (ADD + ADD)
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            // Check if we have at least 2 instructions
            if (pos + 1 >= instrs.size()) return false;

            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Check if both instructions are ADD operations
            if (instr1.opcode != InstructionDecoder::OpType::ADD ||
                instr2.opcode != InstructionDecoder::OpType::ADD) {
                return false;
            }

            // Check if both instructions use immediates
            if (!instr1.uses_immediate || !instr2.uses_immediate) {
                return false;
            }

            // Verify that the destination of the first instruction is both the
            // destination and the first source operand of the second instruction
            if (!InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg) ||
                !InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1)) {
                return false;
            }

            // Get the immediate values
            int64_t imm1 = instr1.immediate;
            int64_t imm2 = instr2.immediate;

            // Calculate the combined immediate and verify it fits in the 12-bit field (0-4095)
            int64_t combined_imm = imm1 + imm2;
            if (combined_imm < 0 || combined_imm > 4095) {
                return false;
            }

            // All checks passed, pattern matches
            return true;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Use integer register numbers from semantic fields
            int dest_reg_num = instr1.dest_reg;
            int src_reg_num = instr1.src_reg1;

            // Get the immediate values
            int64_t imm1 = instr1.immediate;
            int64_t imm2 = instr2.immediate;

            // Calculate the combined immediate
            int64_t combined_imm = imm1 + imm2;

            // Create the new ADD instruction with combined immediate
            Instruction new_instr = Encoder::create_add_imm(dest_reg_num, src_reg_num, static_cast<int>(combined_imm));

            return { new_instr };
        },
        "Fused consecutive ADD instructions with immediates"
    );
}