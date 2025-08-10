#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include "../InstructionComparator.h"

/**
 * @brief Creates a pattern to fuse compare zero and branch instructions.
 * Pattern: CMP Xn, #0 + B.EQ label -> CBZ Xn, label
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createCompareZeroBranchPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: 2 instructions (CMP + B.cond)
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            if (pos + 1 >= instrs.size()) return false;

            const auto& cmp_instr = instrs[pos];
            const auto& br_instr = instrs[pos + 1];

            // Check if first instruction is a compare
            if (cmp_instr.opcode != InstructionDecoder::OpType::CMP) {
                return false;
            }

            // Check if it's a compare with zero (immediate #0 or XZR/WZR register)
            bool is_zero_compare = false;
            if (cmp_instr.uses_immediate) {
                is_zero_compare = (cmp_instr.immediate == 0);
            } else {
                // Use semantic field for src_reg2
                is_zero_compare = (cmp_instr.src_reg2 == 31); // 31 is XZR/WZR
            }

            if (!is_zero_compare) {
                return false;
            }

            // Check if second instruction is a conditional branch (semantic field)
            if (br_instr.opcode != InstructionDecoder::OpType::B_COND) {
                return false;
            }

            // It's a compare with zero followed by a conditional branch - the pattern matches
            return true;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& cmp_instr = instrs[pos];
            const auto& br_instr = instrs[pos + 1];

            int reg = cmp_instr.src_reg1; // The register being compared
            int cond = br_instr.condition_code; // Assume this is an enum or int for the branch condition
            std::string label = br_instr.target_label; // The branch target label

            // Create appropriate instruction based on condition
            Instruction result_instr;

            // Assume Condition::EQ and Condition::NE are the correct enums/ints for equality/inequality
            // You may need to adjust these names to match your codebase
            if (cond == InstructionDecoder::Condition::EQ) {
                // CMP Xn, #0 + B.EQ label -> CBZ Xn, label
                result_instr = EncoderExtended::create_cbz(reg, label);
                if (result_instr.encoding == 0) {
                    return { cmp_instr, br_instr };
                }
                return { result_instr };
            } else if (cond == InstructionDecoder::Condition::NE) {
                // CMP Xn, #0 + B.NE label -> CBNZ Xn, label
                result_instr = EncoderExtended::create_cbnz(reg, label);
                if (result_instr.encoding == 0) {
                    return { cmp_instr, br_instr };
                }
                return { result_instr };
            }

            // For other conditions, keep the original instructions
            return { cmp_instr, br_instr };
        },
        "Compare zero and branch fused"
    );
}