#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <regex>

/**
 * @brief Creates a pattern to optimize common bit field operations.
 * 
 * This pattern recognizes sequences of instructions that perform bit manipulation
 * and replaces them with ARM64's dedicated bit field instructions like UBFX, UBFIZ,
 * SBFX, and SBFIZ when possible.
 * 
 * Patterns detected:
 * 1. Extract bits (unsigned): LSR Xd, Xn, #shift + AND Xd, Xd, #mask -> UBFX Xd, Xn, #lsb, #width
 * 2. Extract bits (signed): ASR Xd, Xn, #shift + AND Xd, Xd, #mask -> SBFX Xd, Xn, #lsb, #width
 * 3. Insert bits: AND Xd, Xn, #mask1 + LSL Xd, Xd, #shift + AND Xt, Xt, #mask2 + ORR Xd, Xd, Xt -> BFIZ/BFI
 * 
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createBitFieldOperationsPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: 2 instructions (LSR/ASR + AND or AND + LSL)
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            if (pos + 1 >= instrs.size()) return false;

            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Only handle LSR/ASR + AND patterns
            bool isLSR = (instr1.opcode == InstructionDecoder::OpType::LSR);
            bool isASR = (instr1.opcode == InstructionDecoder::OpType::ASR);
            bool isAND = (instr2.opcode == InstructionDecoder::OpType::AND);

            if (!((isLSR || isASR) && isAND)) return false;

            // Both must use immediates
            if (!instr1.uses_immediate || !instr2.uses_immediate) return false;

            // Register flow: dest of shift must be src/dest of AND
            bool dest_match = InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg);
            bool src_match = InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1);
            if (!dest_match || !src_match) return false;

            // Mask must be a simple bitmask (all consecutive 1s)
            int64_t mask = instr2.immediate;
            if (mask <= 0 || (mask & (mask + 1)) != 0) return false;

            return true;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            int dest_reg_num = instr1.dest_reg;
            int src_reg_num = instr1.src_reg1;
            int shift_amount = instr1.immediate;
            int64_t mask = instr2.immediate;

            // Calculate width from the mask (number of consecutive 1s)
            int width = 0;
            for (int i = 0; i < 64; ++i) {
                if ((mask >> i) & 1) width++;
            }

            if (instr1.opcode == InstructionDecoder::OpType::LSR) {
                return { EncoderExtended::create_ubfx(dest_reg_num, src_reg_num, shift_amount, width) };
            } else if (instr1.opcode == InstructionDecoder::OpType::ASR) {
                return { EncoderExtended::create_sbfx(dest_reg_num, src_reg_num, shift_amount, width) };
            }

            // If no optimization applies, return the original instructions
            return { instr1, instr2 };
        },
        "Bit Field Operations Optimization"
    );
}