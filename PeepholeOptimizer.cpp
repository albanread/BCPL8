#include "PeepholeOptimizer.h"
#include "optimizer/PeepholePatterns.h"
#include "InstructionComparator.h" // Now using the new comparator
#include "InstructionDecoder.h" // For instruction decoding utilities
#include <iostream>
#include <cassert>
#include <regex>
#include <sstream>

// InstructionPattern implementation
InstructionPattern::InstructionPattern(size_t pattern_size, MatcherFunction matcher_func,
                                       std::function<std::vector<Instruction>(const std::vector<Instruction>&, size_t)> transformer_func,
                                       std::string description)
    : pattern_size_(pattern_size), matcher_(std::move(matcher_func)),
      transformer_(std::move(transformer_func)), description_(std::move(description)) {}

MatchResult InstructionPattern::matches(const std::vector<Instruction>& instructions, size_t position) const {
    // Use the matcher function to determine if the pattern matches
    return matcher_(instructions, position);
}

std::vector<Instruction> InstructionPattern::transform(const std::vector<Instruction>& instructions, size_t position) const {
    // Apply the transformation function to the matched instructions
    return transformer_(instructions, position);
}

// PeepholeOptimizer implementation
PeepholeOptimizer::PeepholeOptimizer(bool enable_tracing)
    : enable_tracing_(enable_tracing) {

        // 1. Simplification and Constant Folding Pass
        addPattern(PeepholePatterns::createConstantFoldingPattern());
        addPattern(PeepholePatterns::createIdentityOperationEliminationPattern());
        addPattern(PeepholePatterns::createRedundantMovePattern());
        addPattern(PeepholePatterns::createSelfMoveEliminationPattern());
        addPattern(PeepholePatterns::createAdrFusionPattern());

        // 2. Strength Reduction Pass
        addPattern(PeepholePatterns::createStrengthReductionPattern());
        addPattern(PeepholePatterns::createMultiplyByPowerOfTwoPattern());
        addPattern(PeepholePatterns::createDivideByPowerOfTwoPattern());

        // 3. Memory Optimization Pass
        addPattern(PeepholePatterns::createLoadAfterStorePattern());
        addPattern(PeepholePatterns::createRedundantLoadEliminationPattern());
        addPattern(PeepholePatterns::createDeadStorePattern());
        addPattern(PeepholePatterns::createRedundantStorePattern());

        // 4. Control Flow Optimization Pass
        addPattern(PeepholePatterns::createCompareZeroBranchPattern());
        addPattern(PeepholePatterns::createBranchChainingPattern());

}

void PeepholeOptimizer::addPattern(std::unique_ptr<InstructionPattern> pattern) {
    patterns_.push_back(std::move(pattern));
}

void PeepholeOptimizer::optimize(InstructionStream& instruction_stream, int max_passes) {
    // Reset statistics
    stats_.clear();

    // Get a copy of the instructions that we can modify
    std::vector<Instruction> instructions = instruction_stream.get_instructions();
    stats_.total_instructions_before = static_cast<int>(instructions.size());

    if (enable_tracing_) {
        std::cout << "\n=== Peephole Optimization ===\n";
        std::cout << "Analyzing " << instructions.size() << " ARM64 instructions...\n";
        std::cout << "Maximum optimization passes: " << max_passes << "\n";
    }

    // Apply exactly max_passes optimization passes
    int pass_count = 0;
    int total_changes = 0;

    while (pass_count < max_passes) {
        bool changes_made = applyOptimizationPass(instructions);
        pass_count++;

        if (changes_made) {
            total_changes++;
        }
        if (!changes_made) {
                   break; // Exit early if the code is stable
        }
        if (enable_tracing_) {
            trace("Completed pass " + std::to_string(pass_count) + "/" + std::to_string(max_passes) +
                  ", changes made: " + (changes_made ? "yes" : "no"));
        }
    }

    // Replace the original instructions with the optimized ones
    instruction_stream.replace_instructions(instructions);

    // Update statistics
    stats_.total_instructions_after = static_cast<int>(instructions.size());

    if (enable_tracing_) {
        std::cout << "Peephole optimization completed " << pass_count << " \n";
        std::cout << "  Passes with changes: " << total_changes << "\n";
        std::cout << "Peephole optimization completed " << pass_count << " passes:\n";
        std::cout << "  Instructions before: " << stats_.total_instructions_before << "\n";
        std::cout << "  Instructions after:  " << stats_.total_instructions_after << "\n";
        std::cout << "  Total optimizations: " << stats_.optimizations_applied << "\n";

        if (stats_.optimizations_applied > 0) {
            std::cout << "  Patterns matched:\n";
            for (const auto& [pattern, count] : stats_.pattern_matches) {
                std::cout << "    - " << pattern << ": " << count << "\n";
            }
        } else {
            std::cout << "  No optimization patterns matched\n";
        }
        std::cout << "==============================\n";
        trace("Detailed peephole optimization trace complete");
    }
}

bool PeepholeOptimizer::applyOptimizationPass(std::vector<Instruction>& instructions) {
    bool any_changes = false;
    size_t pos = 0;

    while (pos < instructions.size()) {
        // Only attempt to optimize instructions that are part of the CODE segment.
        // Skip over any instructions intended for RODATA or DATA segments.
        if (instructions[pos].segment != SegmentType::CODE) {
            pos++;
            continue; // Skip to the next instruction
        }

        bool applied_optimization = false;

        // Skip special instructions (labels, directives, etc.)
        if (isSpecialInstruction(instructions[pos])) {
            pos++;
            continue;
        }

        // Try each pattern
        for (const auto& pattern : patterns_) {
            MatchResult result = pattern->matches(instructions, pos);
            if (result.matched) {
                // Check if applying this optimization would break any label references
                if (wouldBreakLabelReferences(instructions, pos, result.length,
                                             pattern->transform(instructions, pos))) {
                    continue;  // Skip this pattern if it would break references
                }

                // Apply the pattern's transformation
                std::vector<Instruction> replacements = pattern->transform(instructions, pos);

                // Update statistics
                stats_.optimizations_applied++;
                stats_.pattern_matches[pattern->getDescription()]++;

                // Get the original instructions for tracing
                std::vector<Instruction> original_instructions;
                for (size_t i = 0; i < result.length; i++) {
                    original_instructions.push_back(instructions[pos + i]);
                }

                // Use enhanced tracing to show detailed before/after
                if (enable_tracing_) {
                    traceOptimization(
                        pattern->getDescription(),
                        original_instructions,
                        replacements,
                        pos
                    );
                }

                // Replace the original instructions with the optimized ones
                instructions.erase(instructions.begin() + pos,
                                  instructions.begin() + pos + result.length);
                instructions.insert(instructions.begin() + pos,
                                   replacements.begin(), replacements.end());

                applied_optimization = true;
                any_changes = true;
                break;  // Break out of the pattern loop
            }
        }

        // If no optimization was applied, move to the next instruction
        if (!applied_optimization) {
            pos++;
        }
    }

    return any_changes;
}

void PeepholeOptimizer::trace(const std::string& message) const {
    if (enable_tracing_) {
        std::cout << "[Peephole Optimizer] " << message << std::endl;
    }
}

void PeepholeOptimizer::traceOptimization(const std::string& description,
                                         const std::vector<Instruction>& before,
                                         const std::vector<Instruction>& after,
                                         size_t position) const {


    std::cout << "\n[Peephole Optimizer] Applied: " << description << std::endl;
    std::cout << "  Position: " << position << std::endl;

    // Show original instructions
    std::cout << "  Before:" << std::endl;
    for (size_t i = 0; i < before.size(); i++) {
        std::cout << "    " << before[i].assembly_text;
        if (InstructionDecoder::getOpcode(before[i]) != InstructionDecoder::OpType::UNKNOWN) {
            std::cout << "  [Opcode=" << static_cast<int>(InstructionDecoder::getOpcode(before[i]))
                     << ", Dest=" << InstructionDecoder::getDestReg(before[i])
                     << ", Src1=" << InstructionDecoder::getSrcReg1(before[i]);

            if (InstructionDecoder::usesImmediate(before[i])) {
                std::cout << ", Imm=" << InstructionDecoder::getImmediate(before[i]);
            } else if (InstructionDecoder::getSrcReg2(before[i]) != -1) {
                std::cout << ", Src2=" << InstructionDecoder::getSrcReg2(before[i]);
            }

            if (InstructionDecoder::isMemoryOp(before[i])) {
                std::cout << ", Base=" << InstructionDecoder::getBaseReg(before[i])
                         << ", Offset=" << InstructionDecoder::getOffset(before[i]);
            }

            std::cout << "]";
        }
        std::cout << std::endl;
    }

    // Show optimized instructions
    std::cout << "  After:" << std::endl;
    for (size_t i = 0; i < after.size(); i++) {
        std::cout << "    " << after[i].assembly_text;
        if (InstructionDecoder::getOpcode(after[i]) != InstructionDecoder::OpType::UNKNOWN) {
            std::cout << "  [Opcode=" << static_cast<int>(InstructionDecoder::getOpcode(after[i]))
                     << ", Dest=" << InstructionDecoder::getDestReg(after[i])
                     << ", Src1=" << InstructionDecoder::getSrcReg1(after[i]);

            if (InstructionDecoder::usesImmediate(after[i])) {
                std::cout << ", Imm=" << InstructionDecoder::getImmediate(after[i]);
            } else if (InstructionDecoder::getSrcReg2(after[i]) != -1) {
                std::cout << ", Src2=" << InstructionDecoder::getSrcReg2(after[i]);
            }

            if (InstructionDecoder::isMemoryOp(after[i])) {
                std::cout << ", Base=" << InstructionDecoder::getBaseReg(after[i])
                         << ", Offset=" << InstructionDecoder::getOffset(after[i]);
            }

            std::cout << "]";
        }
        std::cout << std::endl;
    }

    std::cout << "  Instruction count: " << before.size() << " -> " << after.size() << std::endl;
    std::cout << std::endl;
}

bool PeepholeOptimizer::isSpecialInstruction(const Instruction& instr) {
    // Labels, data directives, and comments should be preserved
    const std::string& assembly = instr.assembly_text;
    return instr.is_label_definition || instr.is_data_value ||
           assembly.empty() || assembly[0] == ';' || assembly[0] == '.';
}

bool PeepholeOptimizer::wouldBreakLabelReferences(
    const std::vector<Instruction>& instructions,
    size_t start_pos, size_t count,
    const std::vector<Instruction>& replacements) const {

    // Check if any of the instructions being replaced has a label reference
    for (size_t i = 0; i < count && (start_pos + i) < instructions.size(); i++) {
        if (!instructions[start_pos + i].target_label.empty()) {
            // If the target label is not empty, it means this instruction is referenced by a label
            return true;
        }
    }

    return false;
}





// Factory methods for creating common optimization patterns

// Redundant Load Elimination Pattern: LDR Rd, [..]; LDR Rd, [..] => LDR Rd, [..]


// Branch Chaining Pattern: B label1 ... label1: B label2  =>  B label2 ... label1: B label2
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createBranchChainingPattern() {
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
                        if (PeepholeOptimizer::isSpecialInstruction(instrs[j])) {
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

// In PeepholePatterns.cpp

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createRedundantLoadEliminationPattern() {
    return std::make_unique<InstructionPattern>(
        2, // Fixed-window of 2 instructions
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // 1. Both must be LDR instructions.
            if (instr1.opcode != InstructionDecoder::OpType::LDR ||
                instr2.opcode != InstructionDecoder::OpType::LDR) {
                return {false, 0};
            }

            // 2. They must load from the *exact same memory location*.
            if (!InstructionComparator::areSameRegister(instr1.base_reg, instr2.base_reg) ||
                instr1.immediate != instr2.immediate) {
                return {false, 0};
            }

            // 3. CRITICAL FIX: The first load's destination must NOT be used as the base
            //    or index register for the second load. This prevents faulty optimization
            //    of pointer dereferences like LDR X1, [X2]; LDR X3, [X1].
            if (InstructionComparator::areSameRegister(instr1.dest_reg, instr2.base_reg) ||
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg2)) { // src_reg2 for scaled/indexed
                return {false, 0}; // This is a dependency, not a redundant load!
            }

            return {true, 2};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Keep the first load, and replace the second one with a MOV.
            Instruction new_mov = Encoder::create_mov_reg(
                InstructionDecoder::getRegisterName(instr2.dest_reg),
                InstructionDecoder::getRegisterName(instr1.dest_reg)
            );

            return { instr1, new_mov };
        },
        "Redundant Load Elimination"
    );
}


std::unique_ptr<InstructionPattern> PeepholeOptimizer::createRedundantMovePattern() {
    // Pattern 1: mov x0, x1; mov x1, x0  ->  mov x0, x1 (circular moves)
    // Pattern 2: mov Rd1, Rn; mov Rd2, Rd1 -> mov Rd2, Rn (chain of moves)
    // Note: Register numbers are converted to x-register names (e.g., 0 -> x0)
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Check if we have two MOV instructions using semantic information
            if (InstructionDecoder::getOpcode(instr1) != InstructionDecoder::OpType::MOV ||
                InstructionDecoder::getOpcode(instr2) != InstructionDecoder::OpType::MOV) {
                return {false, 0};
            }

            // Check if neither instruction uses immediate values
            if (InstructionDecoder::usesImmediate(instr1) || InstructionDecoder::usesImmediate(instr2)) {
                return {false, 0};
            }

            // Skip instructions with invalid register numbers
            int destReg1 = InstructionDecoder::getDestReg(instr1);
            int srcReg1 = InstructionDecoder::getSrcReg1(instr1);
            int destReg2 = InstructionDecoder::getDestReg(instr2);
            int srcReg2 = InstructionDecoder::getSrcReg1(instr2);

            if (destReg1 < 0 || srcReg1 < 0 || destReg2 < 0 || srcReg2 < 0) {
                return {false, 0};
            }

            // Case 1: Check if second mov undoes the first one (circular moves)
            // bool isCircular = destReg1 == srcReg2 && srcReg1 == destReg2; // Disabled circular move detection

            // Case 2: Check if there's a chain of moves where the destination of the first
            // becomes the source of the second
            bool isChain = destReg1 == srcReg2 && destReg1 != destReg2;

            if (isChain) { // Removed isCircular condition
                return {true, 2};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Check if it's a chain of moves (not circular)
            if (InstructionDecoder::getDestReg(instr1) == InstructionDecoder::getSrcReg1(instr2) &&
                InstructionDecoder::getSrcReg1(instr1) != InstructionDecoder::getDestReg(instr2)) {
                // Create a new MOV instruction: MOV Rd2, Rn
                int srcRegNum = instr1.src_reg1;  // Original source (Rn)
                int dstRegNum = instr2.dest_reg;  // Final destination (Rd2)

                // Skip invalid register numbers
                if (srcRegNum < 0 || dstRegNum < 0) {
                    return { instrs[pos] };  // Keep the first instruction if we can't optimize
                }

                std::string dest_reg_name = InstructionDecoder::getRegisterName(dstRegNum);
                std::string src_reg_name = InstructionDecoder::getRegisterName(srcRegNum);

                // Call the existing Encoder function with the expected string types
                Instruction new_instr = Encoder::create_mov_reg(dest_reg_name, src_reg_name);
                return { new_instr };
            }

            // Circular case - keep only the first instruction
            return { instrs[pos] };
        },
        "Redundant move elimination"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createCopyPropagationPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: MOV followed by a dependent instruction
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& mov_instr = instrs[pos];
            const auto& next_instr = instrs[pos + 1];

            // Check if the first instruction is a MOV
            if (InstructionDecoder::getOpcode(mov_instr) != InstructionDecoder::OpType::MOV) {
                return {false, 0};
            }

            // Check if the destination of the MOV is used as a source in the next instruction
            int mov_dest = InstructionDecoder::getDestReg(mov_instr);
            if (mov_dest < 0 ||
                (InstructionDecoder::getSrcReg1(next_instr) != mov_dest &&
                 InstructionDecoder::getSrcReg2(next_instr) != mov_dest)) {
                return {false, 0};
            }

            // Ensure the next instruction does not write to the MOV's source register
            int mov_src = InstructionDecoder::getSrcReg1(mov_instr);
            if (mov_src >= 0 &&
                (InstructionDecoder::getDestReg(next_instr) == mov_src)) {
                return {false, 0};
            }

            return {true, 2}; // Match found
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& mov_instr = instrs[pos];
            const auto& next_instr = instrs[pos + 1];

            // Create a new version of the next instruction with the MOV's source as its operand
            Instruction optimized_instr = next_instr;
            int mov_src = InstructionDecoder::getSrcReg1(mov_instr);
            if (InstructionDecoder::getSrcReg1(next_instr) == InstructionDecoder::getDestReg(mov_instr)) {
                optimized_instr.src_reg1 = mov_src;
            }
            if (InstructionDecoder::getSrcReg2(next_instr) == InstructionDecoder::getDestReg(mov_instr)) {
                optimized_instr.src_reg2 = mov_src;
            }

            return { optimized_instr }; // Return the optimized instruction
        },
        "Copy propagation (eliminate redundant MOV)"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createLoadAfterStorePattern() {
    // Pattern: str x0, [x1, #offset]; ldr x2, [x1, #offset]  ->  str x0, [x1, #offset]; mov x2, x0
    // This optimizes memory access by replacing a load with a direct register-to-register move
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Check if it's a store followed by a load using semantic information
            if (InstructionDecoder::getOpcode(instr1) != InstructionDecoder::OpType::STR ||
                InstructionDecoder::getOpcode(instr2) != InstructionDecoder::OpType::LDR) {
                return {false, 0};
            }

            // Both instructions must be memory operations
            if (!InstructionDecoder::isMemoryOp(instr1) || !InstructionDecoder::isMemoryOp(instr2)) {
                return {false, 0};
            }

            // Check if they access the same memory location (same base register and offset)
            if (InstructionDecoder::getBaseReg(instr1) == InstructionDecoder::getBaseReg(instr2) &&
                InstructionDecoder::getOffset(instr1) == InstructionDecoder::getOffset(instr2)) {
                return {true, 2};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& store_instr = instrs[pos];
            const auto& load_instr = instrs[pos + 1];

            // Get source and destination register numbers directly from semantic fields
            int src_reg_num = store_instr.src_reg1;
            int dest_reg_num = load_instr.dest_reg;

            // Safety check: Don't optimize if it's a move to the same register.
            if (InstructionComparator::areSameRegister(src_reg_num, dest_reg_num)) {
                return { store_instr, load_instr };
            }

            // Convert integer IDs back to string names locally
            std::string dest_reg_name = InstructionDecoder::getRegisterName(dest_reg_num);
            std::string src_reg_name = InstructionDecoder::getRegisterName(src_reg_num);

            // Call the existing Encoder function with the expected string types
            Instruction new_mov = Encoder::create_mov_reg(dest_reg_name, src_reg_name);

            // Return the original store followed by the new, faster MOV instruction
            return { store_instr, new_mov };
        },
        "Load after store optimization with safe register move"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createDeadStorePattern() {
    // Pattern: str x0, [x1, #offset]; str x2, [x1, #offset]  ->  str x2, [x1, #offset]
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Check if both instructions are store operations
            if (InstructionDecoder::getOpcode(instr1) != InstructionDecoder::OpType::STR ||
                InstructionDecoder::getOpcode(instr2) != InstructionDecoder::OpType::STR) {
                return {false, 0};
            }

            // Both instructions must be memory operations
            if (!InstructionDecoder::isMemoryOp(instr1) || !InstructionDecoder::isMemoryOp(instr2)) {
                return {false, 0};
            }

            // Check if they store to the same memory location (same base register and offset)
            if (InstructionComparator::haveSameMemoryOperand(instr1, instr2)) {
                return {true, 2};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Keep only the second store instruction
            return { instrs[pos + 1] };
        },
        "Dead store elimination"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createRedundantComparePattern() {
    // Pattern: cmp x0, #0; cmp x0, #0  ->  cmp x0, #0
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Check if both are compare instructions using semantic information
            if (InstructionDecoder::getOpcode(instr1) != InstructionDecoder::OpType::CMP ||
                InstructionDecoder::getOpcode(instr2) != InstructionDecoder::OpType::CMP) {
                return {false, 0};
            }

            // If comparing the same register with the same value, the second is redundant
            if (InstructionDecoder::usesImmediate(instr1) && InstructionDecoder::usesImmediate(instr2)) {
                if (InstructionDecoder::getSrcReg1(instr1) == InstructionDecoder::getSrcReg1(instr2) &&
                    InstructionDecoder::getImmediate(instr1) == InstructionDecoder::getImmediate(instr2)) {
                    return {true, 2};
                }
            } else if (!InstructionDecoder::usesImmediate(instr1) && !InstructionDecoder::usesImmediate(instr2)) {
                if (InstructionDecoder::getSrcReg1(instr1) == InstructionDecoder::getSrcReg1(instr2) &&
                    InstructionDecoder::getSrcReg2(instr1) == InstructionDecoder::getSrcReg2(instr2)) {
                    return {true, 2};
                }
            }

            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Keep only one compare instruction
            return { instrs[pos] };
        },
        "Redundant compare elimination"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createConstantFoldingPattern() {
    // Pattern: mov x0, #c1; add x0, x0, #c2  ->  mov x0, #(c1+c2)
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Check if we have a mov followed by an add with immediate
            if (InstructionDecoder::getOpcode(instr1) != InstructionDecoder::OpType::MOV ||
                InstructionDecoder::getOpcode(instr2) != InstructionDecoder::OpType::ADD) {
                return {false, 0};
            }

            // Both instructions must use immediate values
            if (!InstructionDecoder::usesImmediate(instr1) || !InstructionDecoder::usesImmediate(instr2)) {
                return {false, 0};
            }

            // Check if same register used in both instructions
            // MOV rd, #imm followed by ADD rd, rd, #imm
            if (InstructionDecoder::getDestReg(instr1) == InstructionDecoder::getDestReg(instr2) &&
                InstructionDecoder::getDestReg(instr1) == InstructionDecoder::getSrcReg1(instr2)) {
                return {true, 2};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Extract values and compute the new constant
            const auto& mov_instr = instrs[pos];
            const auto& add_instr = instrs[pos + 1];

            int reg_num = mov_instr.dest_reg;
            int64_t val1 = mov_instr.immediate;
            int64_t val2 = add_instr.immediate;
            int64_t result = val1 + val2;

            // Only allow folding if result fits in a single MOVZ (16-bit unsigned)
            if (result < 0 || result > 65535) {
                return { mov_instr, add_instr };
            }

            // Convert the integer register number to its string name first.
            std::string reg_name = InstructionDecoder::getRegisterName(reg_num);

            // Use the existing Encoder function for MOVZ with the correct string type
            Instruction new_instr = Encoder::create_movz_imm(reg_name, result);

            return { new_instr };
        },
        "Constant folding (add)"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createStrengthReductionPattern() {
    // Pattern: mul x0, x1, #2  ->  add x0, x1, x1
    return std::make_unique<InstructionPattern>(
        1,  // Pattern size
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];

            // Check if we have a multiply by 2 using semantic information
            if (InstructionDecoder::getOpcode(instr) == InstructionDecoder::OpType::MUL &&
                InstructionDecoder::usesImmediate(instr) &&
                InstructionDecoder::getImmediate(instr) == 2) {
                return {true, 1};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr = instrs[pos];
            int dest_reg = instr.dest_reg;
            int src_reg = instr.src_reg1;

            // Convert integer IDs back to string names locally
            std::string dest_reg_name = InstructionDecoder::getRegisterName(dest_reg);
            std::string src_reg_name = InstructionDecoder::getRegisterName(src_reg);

            // Call the existing Encoder function with the expected string types
            Instruction new_instr = Encoder::create_add_reg(dest_reg_name, src_reg_name, src_reg_name);

            return { new_instr };
        },
        "Multiply by 2 strength reduction"
    );
}

/**
 * @brief Creates a pattern to eliminate identity operations.
 * * This pattern recognizes instructions that have no effect on program state and
 * replaces them with simpler alternatives or removes them entirely.
 * * Patterns detected:
 * 1. ADD/SUB with zero immediate: ADD Xd, Xn, #0 -> MOV Xd, Xn
 * 2. MUL/DIV by one immediate: MUL Xd, Xn, #1 -> MOV Xd, Xn
 * 3. Self-subtraction: SUB Xd, Xn, Xn -> MOVZ Xd, #0
 * * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createIdentityOperationEliminationPattern() {
    return std::make_unique<InstructionPattern>(
        1, // Pattern size of 1 instruction
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos >= instrs.size()) return {false, 0};
            const auto& instr = instrs[pos];
            auto opcode = instr.opcode;

            // Case 1: ADD/SUB with zero immediate
            if ((opcode == InstructionDecoder::OpType::ADD || opcode == InstructionDecoder::OpType::SUB) &&
                instr.uses_immediate && instr.immediate == 0) {
                return {true, 1};
            }

            // Case 2: MUL/DIV by one immediate
            if ((opcode == InstructionDecoder::OpType::MUL || opcode == InstructionDecoder::OpType::SDIV) &&
                instr.uses_immediate && instr.immediate == 1) {
                return {true, 1};
            }

            // Case 3: Self-subtraction (SUB Xd, Xn, Xn)
            if (opcode == InstructionDecoder::OpType::SUB && !instr.uses_immediate &&
                InstructionComparator::areSameRegister(instr.src_reg1, instr.src_reg2)) {
                return {true, 1};
            }

            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr = instrs[pos];
            auto opcode = instr.opcode;

            // Case 1 & 2: ADD/SUB #0, MUL/DIV #1 -> MOV Xd, Xn
            if ((opcode == InstructionDecoder::OpType::ADD || opcode == InstructionDecoder::OpType::SUB ||
                 opcode == InstructionDecoder::OpType::MUL || opcode == InstructionDecoder::OpType::SDIV) &&
                instr.uses_immediate) {

                return { Encoder::create_mov_reg(
                    InstructionDecoder::getRegisterName(instr.dest_reg),
                    InstructionDecoder::getRegisterName(instr.src_reg1)
                )};
            }

            // Case 3: Self-subtraction (SUB Xd, Xn, Xn) -> MOVZ Xd, #0
            if (opcode == InstructionDecoder::OpType::SUB &&
                InstructionComparator::areSameRegister(instr.src_reg1, instr.src_reg2)) {

                return { Encoder::create_movz_imm(
                    InstructionDecoder::getRegisterName(instr.dest_reg), 0
                )};
            }

            // Should not be reached if matcher is correct, but return original as a fallback.
            return { instr };
        },
        "Identity Operation Elimination"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createRedundantStorePattern() {
    return std::make_unique<InstructionPattern>(
        2,
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            if (InstructionDecoder::getOpcode(instr1) == InstructionDecoder::OpType::STR &&
                InstructionDecoder::getOpcode(instr2) == InstructionDecoder::OpType::STR &&
                InstructionComparator::haveSameMemoryOperand(instr1, instr2)) {
                return {true, 2};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // The second store is the one that is kept.
            return { instrs[pos + 1] };
        },
        "Redundant Store Elimination same stores"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createAdrFusionPattern() {
    return std::make_unique<InstructionPattern>(
        2,
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            if (InstructionDecoder::getOpcode(instr1) == InstructionDecoder::OpType::ADRP &&
                InstructionDecoder::getOpcode(instr2) == InstructionDecoder::OpType::ADD &&
                InstructionDecoder::usesImmediate(instr2) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1)) {
                return {true, 2};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& adrp_instr = instrs[pos];
            // In a real implementation, you would need to resolve the label's address
            // and calculate the offset from the current instruction's position.
            // If the offset is within the ADR's range, create an ADR instruction.
            // For now, we'll just return the original instructions.
            return { adrp_instr, instrs[pos + 1] };
        },
        "ADRP/ADD to ADR Fusion"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createSelfMoveEliminationPattern() {
    return std::make_unique<InstructionPattern>(
        1,
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];
            if (InstructionDecoder::getOpcode(instr) == InstructionDecoder::OpType::MOV &&
                !InstructionDecoder::usesImmediate(instr) &&
                InstructionComparator::areSameRegister(instr.dest_reg, instr.src_reg1)) {
                return {true, 1};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Return an empty vector to remove the instruction
            return {};
        },
        "Self-Move Elimination"
    );
}
