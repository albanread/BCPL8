#include "../PeepholePatterns.h"
#include "../../Encoder.h"
#include "../../InstructionDecoder.h"
#include <memory>
#include <string>

namespace PeepholePatterns {

// Compare Zero Branch Pattern: Optimize CMP Xn, #0; B.cond label
std::unique_ptr<InstructionPattern> createCompareZeroBranchPattern() {
    return std::make_unique<InstructionPattern>(
        2, // Pattern size: 2 instructions (CMP followed by B.cond)
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& cmp_instr = instrs[pos];
            const auto& branch_instr = instrs[pos + 1];

            if (InstructionDecoder::getOpcode(cmp_instr) != InstructionDecoder::OpType::CMP)
                return {false, 0};
            if (InstructionDecoder::getOpcode(branch_instr) != InstructionDecoder::OpType::B_COND)
                return {false, 0};

            // Only optimize CMP Xn, #0
            if (!InstructionDecoder::usesImmediate(cmp_instr) || InstructionDecoder::getImmediate(cmp_instr) != 0)
                return {false, 0};

            return {true, 2};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // For now, just return the original instructions.
            // In a more advanced optimizer, you could replace with CBZ/CBNZ if the condition matches.
            return {instrs[pos], instrs[pos + 1]};
        },
        "Compare zero branch pattern (CMP Xn, #0; B.cond label)"
    );
}

// Branch Chaining Pattern: B label1 ... label1: B label2  =>  B label2 ... label1: B label2
std::unique_ptr<InstructionPattern> createBranchChainingPattern() {
    return std::make_unique<InstructionPattern>(
        1, // Pattern size: only targets the first branch instruction
        // Matcher: Finds a 'B label1' where 'label1' also contains a 'B' instruction
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& branch_instr = instrs[pos];
            if (InstructionDecoder::getOpcode(branch_instr) != InstructionDecoder::OpType::B) {
                return {false, 0};
            }

            std::string label1 = branch_instr.branch_target;
            if (label1.empty()) {
                return {false, 0};
            }

            // Find the definition of the target label
            for (size_t i = 0; i < instrs.size(); ++i) {
                if (instrs[i].is_label_definition && instrs[i].label == label1) {
                    // Scan forward to find the first non-special instruction
                    for (size_t j = i + 1; j < instrs.size(); ++j) {
                        if (instrs[j].is_label_definition || instrs[j].is_data_value ||
                            instrs[j].assembly_text.empty() || instrs[j].assembly_text[0] == ';' || instrs[j].assembly_text[0] == '.') {
                            continue; // Skip special instructions
                        }
                        // Check if the first real instruction is an unconditional branch
                        if (InstructionDecoder::getOpcode(instrs[j]) == InstructionDecoder::OpType::B) {
                            return {true, 1}; // Match found
                        }
                        break; // Stop if it's not a branch
                    }
                    break; // Found the label, no need to search further
                }
            }
            return {false, 0};
        },
        // Transformer: Replaces 'B label1' with 'B label2'
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& branch_instr = instrs[pos];
            std::string label1 = branch_instr.branch_target;
            const Instruction* next_branch = nullptr;

            // Redo the search to find the second branch's target
            for (size_t i = 0; i < instrs.size(); ++i) {
                if (instrs[i].is_label_definition && instrs[i].label == label1) {
                    if (i + 1 < instrs.size()) {
                        next_branch = &instrs[i + 1];
                    }
                    break;
                }
            }

            // If the second branch was found, create the new optimized instruction
            if (next_branch && InstructionDecoder::getOpcode(*next_branch) == InstructionDecoder::OpType::B) {
                Instruction new_branch = branch_instr;
                new_branch.branch_target = next_branch->branch_target; // Use the final target
                return { new_branch };
            }

            // If something went wrong, return the original instruction
            return { branch_instr };
        },
        "Branch chaining (eliminate intermediate unconditional branch)"
    );
}

} // namespace PeepholePatterns