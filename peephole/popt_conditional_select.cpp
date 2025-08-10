#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <regex>

/**
 * @brief Creates a pattern to optimize conditional select (CSEL) instructions.
 * 
 * Optimizes the following patterns:
 * 1. CSEL Xd, Xn, #0, cond  ->  CSEL Xd, Xn, XZR, cond  ->  CSINV Xd, Xn, XZR, !cond
 * 2. CSEL Xd, #0, Xm, cond  ->  CSEL Xd, XZR, Xm, cond  ->  CSINV Xd, Xm, XZR, !cond
 * 3. CSEL Xd, Xn, Xn, cond  ->  MOV Xd, Xn (conditional select with same register is redundant)
 * 
 * This reduces instruction count and improves code efficiency by replacing conditional
 * selects with more specific conditional instructions when possible.
 * 
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createConditionalSelectPattern() {
    return std::make_unique<InstructionPattern>(
        1,  // Pattern size: 1 instruction (CSEL)
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            if (pos >= instrs.size()) return false;
            const auto& instr = instrs[pos];
            if (instr.opcode != InstructionDecoder::OpType::CSEL) return false;

            // Pattern 3: CSEL Xd, Xn, Xn, cond -> MOV Xd, Xn
            if (InstructionComparator::areSameRegister(instr.src_reg1, instr.src_reg2)) {
                return true;
            }

            // Pattern 1 & 2: CSEL involving the zero register
            bool src1_is_zr = (instr.src_reg1 == 31); // 31 is the encoding for XZR/WZR
            bool src2_is_zr = (instr.src_reg2 == 31);
            if (src1_is_zr || src2_is_zr) {
                return true;
            }
            return false;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr = instrs[pos];
            // Pattern 3: CSEL Xd, Xn, Xn, cond -> MOV Xd, Xn
            if (InstructionComparator::areSameRegister(instr.src_reg1, instr.src_reg2)) {
                return { Encoder::create_mov_reg(instr.dest_reg, instr.src_reg1) };
            }
            // Pattern 1 & 2: CSEL involving the zero register
            bool src1_is_zr = (instr.src_reg1 == 31);
            bool src2_is_zr = (instr.src_reg2 == 31);
            if (src2_is_zr) {
                // CSEL Xd, Xn, XZR, cond -> CSINV Xd, Xn, XZR, !cond
                // This assumes EncoderExtended::create_csinv can take integer registers and a negated condition code
                // You may need to implement condition code negation as a helper.
                int cond = instr.condition_code; // Assuming this field exists
                int neg_cond = EncoderExtended::negate_condition(cond);
                return { EncoderExtended::create_csinv(instr.dest_reg, instr.src_reg1, 31, neg_cond) };
            }
            if (src1_is_zr) {
                // CSEL Xd, XZR, Xm, cond -> CSINV Xd, Xm, XZR, cond
                int cond = instr.condition_code;
                return { EncoderExtended::create_csinv(instr.dest_reg, instr.src_reg2, 31, cond) };
            }
            // Default: return the original instruction
            return { instr };
        },
        "Conditional select optimization"
    );
}