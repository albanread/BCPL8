#include "PeepholeOptimizer.h"
#include "InstructionComparator.h" // Now using the new comparator
#include "InstructionDecoder.h"    // For instruction decoding utilities
#include <cassert>
#include <iostream>
#include <regex>
#include <sstream>

// InstructionPattern implementation
InstructionPattern::InstructionPattern(
    size_t pattern_size, MatcherFunction matcher_func,
    std::function<std::vector<Instruction>(const std::vector<Instruction> &,
                                           size_t)>
        transformer_func,
    std::string description)
    : pattern_size_(pattern_size), matcher_(std::move(matcher_func)),
      transformer_(std::move(transformer_func)),
      description_(std::move(description)) {}

MatchResult
InstructionPattern::matches(const std::vector<Instruction> &instructions,
                            size_t position) const {
  // Use the matcher function to determine if the pattern matches
  return matcher_(instructions, position);
}

std::vector<Instruction>
InstructionPattern::transform(const std::vector<Instruction> &instructions,
                              size_t position) const {
  // Apply the transformation function to the matched instructions
  return transformer_(instructions, position);
}

// PeepholeOptimizer implementation
PeepholeOptimizer::PeepholeOptimizer(bool enable_tracing)
    : enable_tracing_(enable_tracing) {
  // Add built-in optimization patterns
  addPattern(createRedundantMovePattern());
  addPattern(createAdjacentLoadStoreForwardingPattern());
  addPattern(createLoadStoreForwardingPatternSimple());
  addPattern(createLoadAfterStorePattern());
  addPattern(createDeadStorePattern());
  addPattern(createRedundantComparePattern());
  addPattern(createConstantFoldingPattern());
  addPattern(createStrengthReductionPattern());
  addPattern(createMovzMovFusionPattern());
  addPattern(createRedundantIndexCalculationPattern());



  // Add new advanced optimization patterns
  addPattern(createRedundantLoadEliminationPattern());
  addPattern(createBranchChainingPattern());
  addPattern(createMultiplyByPowerOfTwoPattern());
  addPattern(createDivideByPowerOfTwoPattern());
  addPattern(createCompareZeroBranchPattern());
}

void PeepholeOptimizer::addPattern(
    std::unique_ptr<InstructionPattern> pattern) {
  patterns_.push_back(std::move(pattern));
}

void PeepholeOptimizer::optimize(InstructionStream &instruction_stream,
                                 int max_passes) {
  // Reset statistics
  stats_.clear();

  // Get a copy of the instructions that we can modify
  std::vector<Instruction> instructions = instruction_stream.get_instructions();
  stats_.total_instructions_before = static_cast<int>(instructions.size());

  std::cout << "\n=== Peephole Optimization ===\n";
  std::cout << "Analyzing " << instructions.size()
            << " ARM64 instructions...\n";
  std::cout << "Maximum optimization passes: " << max_passes << "\n";

  // Apply exactly max_passes optimization passes
  int pass_count = 0;
  int total_changes = 0;

  while (pass_count < 20) {
    bool changes_made = applyOptimizationPass(instructions);
    pass_count++;

    if (changes_made) {
      total_changes++;
    }

    if (enable_tracing_) {
      trace("Completed pass " + std::to_string(pass_count) + "/" +
            std::to_string(max_passes) +
            ", changes made: " + (changes_made ? "yes" : "no"));
    }
  }

  // Replace the original instructions with the optimized ones
  instruction_stream.replace_instructions(instructions);

  // Update statistics
  stats_.total_instructions_after = static_cast<int>(instructions.size());

  // Always show optimization results, regardless of trace setting
  std::cout << "Peephole optimization completed " << pass_count
            << " passes (out of 20 requested):\n";
  std::cout << "  Passes with changes: " << total_changes << "\n";
  std::cout << "Peephole optimization completed " << pass_count << " passes:\n";
  std::cout << "  Instructions before: " << stats_.total_instructions_before
            << "\n";
  std::cout << "  Instructions after:  " << stats_.total_instructions_after
            << "\n";
  std::cout << "  Total optimizations: " << stats_.optimizations_applied
            << "\n";

  if (stats_.optimizations_applied > 0) {
    std::cout << "  Patterns matched:\n";
    for (const auto &[pattern, count] : stats_.pattern_matches) {
      std::cout << "    - " << pattern << ": " << count << "\n";
    }
  } else {
    std::cout << "  No optimization patterns matched\n";
  }
  std::cout << "==============================\n";

  if (enable_tracing_) {
    trace("Detailed peephole optimization trace complete");
  }
}

bool PeepholeOptimizer::applyOptimizationPass(
    std::vector<Instruction> &instructions) {
  bool any_changes = false;
  size_t pos = 0;

  while (pos < instructions.size()) {
    bool applied_optimization = false;

    // Skip special instructions (labels, directives, etc.)
    if (isSpecialInstruction(instructions[pos])) {
      pos++;
      continue;
    }

    // Try each pattern
    for (const auto &pattern : patterns_) {
      MatchResult result = pattern->matches(instructions, pos);
      if (result.matched) {
        // Check if applying this optimization would break any label references
        if (wouldBreakLabelReferences(instructions, pos, result.length,
                                      pattern->transform(instructions, pos))) {
          continue; // Skip this pattern if it would break references
        }

        // Apply the pattern's transformation
        std::vector<Instruction> replacements =
            pattern->transform(instructions, pos);

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
          traceOptimization(pattern->getDescription(), original_instructions,
                            replacements, pos);
        }

        // Replace the original instructions with the optimized ones
        instructions.erase(instructions.begin() + pos,
                           instructions.begin() + pos + result.length);
        instructions.insert(instructions.begin() + pos, replacements.begin(),
                            replacements.end());

        applied_optimization = true;
        any_changes = true;
        break; // Break out of the pattern loop
      }
    }

    // If no optimization was applied, move to the next instruction
    if (!applied_optimization) {
      pos++;
    }
  }

  return any_changes;
}

void PeepholeOptimizer::trace(const std::string &message) const {
  if (enable_tracing_) {
    std::cout << "[Peephole Optimizer] " << message << std::endl;
  }
}

void PeepholeOptimizer::traceOptimization(
    const std::string &description, const std::vector<Instruction> &before,
    const std::vector<Instruction> &after, size_t position) const {

  std::cout << "\n[Peephole Optimizer] Applied: " << description << std::endl;
  std::cout << "  Position: " << position << std::endl;

  // Show original instructions
  std::cout << "  Before:" << std::endl;
  for (size_t i = 0; i < before.size(); i++) {
    std::cout << "    " << before[i].assembly_text;
    if (InstructionDecoder::getOpcode(before[i]) !=
        InstructionDecoder::OpType::UNKNOWN) {
      std::cout << "  [Opcode="
                << static_cast<int>(InstructionDecoder::getOpcode(before[i]))
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
    if (InstructionDecoder::getOpcode(after[i]) !=
        InstructionDecoder::OpType::UNKNOWN) {
      std::cout << "  [Opcode="
                << static_cast<int>(InstructionDecoder::getOpcode(after[i]))
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

  std::cout << "  Instruction count: " << before.size() << " -> "
            << after.size() << std::endl;
  std::cout << std::endl;
}

bool PeepholeOptimizer::isSpecialInstruction(const Instruction &instr) {
  // Labels, data directives, and comments should be preserved
  const std::string &assembly = instr.assembly_text;
  return instr.is_label_definition || instr.is_data_value || assembly.empty() ||
         assembly[0] == ';' || assembly[0] == '.';
}

bool PeepholeOptimizer::wouldBreakLabelReferences(
    const std::vector<Instruction> &instructions, size_t start_pos,
    size_t count, const std::vector<Instruction> &replacements) const {

  // Check if any of the instructions being replaced has a label reference
  for (size_t i = 0; i < count && (start_pos + i) < instructions.size(); i++) {
    if (!instructions[start_pos + i].target_label.empty()) {
      // If the target label is not empty, it means this instruction is
      // referenced by a label
      return true;
    }
  }

  return false;
}

// Factory methods for creating common optimization patterns

// Redundant Load Elimination Pattern: LDR Rd, [..]; LDR Rd, [..] => LDR Rd,
// [..]

// Branch Chaining Pattern: B label1 ... label1: B label2  =>  B label2 ...
// label1: B label2
std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createBranchChainingPattern() {
  return std::make_unique<InstructionPattern>(
      1, // Pattern size: only targets the first branch instruction
      // Matcher: Finds a 'B label1' where 'label1' also contains a 'B'
      // instruction
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &branch_instr = instrs[pos];
        if (InstructionDecoder::getOpcode(branch_instr) !=
            InstructionDecoder::OpType::B) {
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
              if (InstructionDecoder::getOpcode(instrs[j]) ==
                  InstructionDecoder::OpType::B) {
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
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        const auto &branch_instr = instrs[pos];
        std::string label1 = branch_instr.branch_target;
        const Instruction *next_branch = nullptr;

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
        if (next_branch && InstructionDecoder::getOpcode(*next_branch) ==
                               InstructionDecoder::OpType::B) {
          Instruction new_branch = branch_instr;
          new_branch.branch_target =
              next_branch->branch_target; // Use the final target
          return {new_branch};
        }

        // If something went wrong, return the original instruction
        return {branch_instr};
      },
      "Branch chaining (eliminate intermediate unconditional branch)");
}

// Redundant Load Elimination Pattern: LDR Rd, [..]; LDR Rd, [..] => LDR Rd,
// [..]
std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createRedundantLoadEliminationPattern() {
  return std::make_unique<InstructionPattern>(
      2,
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];
        // Match two consecutive loads to the same register
        auto op1 = InstructionDecoder::getOpcode(instr1);
        auto op2 = InstructionDecoder::getOpcode(instr2);

        // Check if both instructions are a known load type.
        bool isLoad1 = (op1 == InstructionDecoder::OpType::LDR ||
                        op1 == InstructionDecoder::OpType::LDR_FP);
        bool isLoad2 = (op2 == InstructionDecoder::OpType::LDR ||
                        op2 == InstructionDecoder::OpType::LDR_FP);

        if (isLoad1 && isLoad2) {
          // Check if they are the SAME type of load and write to the SAME
          // register.
          if (op1 == op2 && InstructionDecoder::getDestReg(instr1) ==
                                InstructionDecoder::getDestReg(instr2)) {
            return {true, 2};
          }
        }
        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        // Keep only the second load
        return {instrs[pos + 1]};
      },
      "Redundant load elimination");
}

std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createRedundantMovePattern() {
  // Pattern 1: mov x0, x1; mov x1, x0  ->  mov x0, x1 (circular moves)
  // Pattern 2: mov Rd1, Rn; mov Rd2, Rd1 -> mov Rd2, Rn (chain of moves)
  // Note: Register numbers are converted to x-register names (e.g., 0 -> x0)
  return std::make_unique<InstructionPattern>(
      2, // Pattern size
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];

        // Check if we have two MOV instructions using semantic information
        if (InstructionDecoder::getOpcode(instr1) !=
                InstructionDecoder::OpType::MOV ||
            InstructionDecoder::getOpcode(instr2) !=
                InstructionDecoder::OpType::MOV) {
          return {false, 0};
        }

        // Check if neither instruction uses immediate values
        if (InstructionDecoder::usesImmediate(instr1) ||
            InstructionDecoder::usesImmediate(instr2)) {
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
        // bool isCircular = destReg1 == srcReg2 && srcReg1 == destReg2; //
        // Disabled circular move detection

        // Case 2: Check if there's a chain of moves where the destination of
        // the first becomes the source of the second
        bool isChain = destReg1 == srcReg2 && destReg1 != destReg2;

        if (isChain) { // Removed isCircular condition
          return {true, 2};
        }
        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];

        // Check if it's a chain of moves (not circular)
        if (InstructionDecoder::getDestReg(instr1) ==
                InstructionDecoder::getSrcReg1(instr2) &&
            InstructionDecoder::getSrcReg1(instr1) !=
                InstructionDecoder::getDestReg(instr2)) {
          // Create a new MOV instruction: MOV Rd2, Rn
          int srcRegNum = instr1.src_reg1; // Original source (Rn)
          int dstRegNum = instr2.dest_reg; // Final destination (Rd2)

          // Skip invalid register numbers
          if (srcRegNum < 0 || dstRegNum < 0) {
            return {
                instrs[pos]}; // Keep the first instruction if we can't optimize
          }

          std::string dest_reg_name =
              InstructionDecoder::getRegisterName(dstRegNum);
          std::string src_reg_name =
              InstructionDecoder::getRegisterName(srcRegNum);

          // Call the existing Encoder function with the expected string types
          Instruction new_instr =
              Encoder::create_mov_reg(dest_reg_name, src_reg_name);
          return {new_instr};
        }

        // Circular case - keep only the first instruction
        return {instrs[pos]};
      },
      "Redundant move elimination");
}

std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createCopyPropagationPattern() {
  return std::make_unique<InstructionPattern>(
      2, // Pattern size: MOV followed by a dependent instruction
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &mov_instr = instrs[pos];
        const auto &next_instr = instrs[pos + 1];

        // Check if the first instruction is a MOV
        if (InstructionDecoder::getOpcode(mov_instr) !=
            InstructionDecoder::OpType::MOV) {
          return {false, 0};
        }

        // Check if the destination of the MOV is used as a source in the next
        // instruction
        int mov_dest = InstructionDecoder::getDestReg(mov_instr);
        if (mov_dest < 0 ||
            (InstructionDecoder::getSrcReg1(next_instr) != mov_dest &&
             InstructionDecoder::getSrcReg2(next_instr) != mov_dest)) {
          return {false, 0};
        }

        // Ensure the next instruction does not write to the MOV's source
        // register
        int mov_src = InstructionDecoder::getSrcReg1(mov_instr);
        if (mov_src >= 0 &&
            (InstructionDecoder::getDestReg(next_instr) == mov_src)) {
          return {false, 0};
        }

        return {true, 2}; // Match found
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        const auto &mov_instr = instrs[pos];
        const auto &next_instr = instrs[pos + 1];

        // Create a new version of the next instruction with the MOV's source as
        // its operand
        Instruction optimized_instr = next_instr;
        int mov_src = InstructionDecoder::getSrcReg1(mov_instr);
        if (InstructionDecoder::getSrcReg1(next_instr) ==
            InstructionDecoder::getDestReg(mov_instr)) {
          optimized_instr.src_reg1 = mov_src;
        }
        if (InstructionDecoder::getSrcReg2(next_instr) ==
            InstructionDecoder::getDestReg(mov_instr)) {
          optimized_instr.src_reg2 = mov_src;
        }

        return {optimized_instr}; // Return the optimized instruction
      },
      "Copy propagation (eliminate redundant MOV)");
}

std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createLoadAfterStorePattern() {
  // Pattern: str x0, [x1, #offset]; ldr x2, [x1, #offset]  ->  str x0, [x1,
  // #offset]; mov x2, x0 This optimizes memory access by replacing a load with
  // a direct register-to-register move
  return std::make_unique<InstructionPattern>(
      2, // Pattern size
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];

        // Check if it's a store followed by a load using semantic information
        if (InstructionDecoder::getOpcode(instr1) !=
                InstructionDecoder::OpType::STR ||
            InstructionDecoder::getOpcode(instr2) !=
                InstructionDecoder::OpType::LDR) {
          return {false, 0};
        }

        // Both instructions must be memory operations
        if (!InstructionDecoder::isMemoryOp(instr1) ||
            !InstructionDecoder::isMemoryOp(instr2)) {
          return {false, 0};
        }

        // Check if they access the same memory location (same base register and
        // offset)
        if (InstructionDecoder::getBaseReg(instr1) ==
                InstructionDecoder::getBaseReg(instr2) &&
            InstructionDecoder::getOffset(instr1) ==
                InstructionDecoder::getOffset(instr2)) {
          return {true, 2};
        }
        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        const auto &store_instr = instrs[pos];
        const auto &load_instr = instrs[pos + 1];

        // Get source and destination register numbers directly from semantic
        // fields
        int src_reg_num = store_instr.src_reg1;
        int dest_reg_num = load_instr.dest_reg;

        // Safety check: Don't optimize if it's a move to the same register.
        if (InstructionComparator::areSameRegister(src_reg_num, dest_reg_num)) {
          return {store_instr, load_instr};
        }

        // Convert integer IDs back to string names locally
        std::string dest_reg_name =
            InstructionDecoder::getRegisterName(dest_reg_num);
        std::string src_reg_name =
            InstructionDecoder::getRegisterName(src_reg_num);

        // Call the existing Encoder function with the expected string types
        Instruction new_mov =
            Encoder::create_mov_reg(dest_reg_name, src_reg_name);

        // Return the original store followed by the new, faster MOV instruction
        return {store_instr, new_mov};
      },
      "Load after store optimization with safe register move");
}

std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createDeadStorePattern() {
  // Pattern: str x0, [x1, #offset]; str x2, [x1, #offset]  ->  str x2, [x1,
  // #offset]
  return std::make_unique<InstructionPattern>(
      2, // Pattern size
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];

        // Check if both instructions are store operations
        if (InstructionDecoder::getOpcode(instr1) !=
                InstructionDecoder::OpType::STR ||
            InstructionDecoder::getOpcode(instr2) !=
                InstructionDecoder::OpType::STR) {
          return {false, 0};
        }

        // Both instructions must be memory operations
        if (!InstructionDecoder::isMemoryOp(instr1) ||
            !InstructionDecoder::isMemoryOp(instr2)) {
          return {false, 0};
        }

        // Check if they store to the same memory location (same base register
        // and offset)
        if (InstructionComparator::haveSameMemoryOperand(instr1, instr2)) {
          return {true, 2};
        }
        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        // Keep only the second store instruction
        return {instrs[pos + 1]};
      },
      "Dead store elimination");
}

std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createRedundantComparePattern() {
  // Pattern: cmp x0, #0; cmp x0, #0  ->  cmp x0, #0
  return std::make_unique<InstructionPattern>(
      2, // Pattern size
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];

        // Check if both are compare instructions using semantic information
        if (InstructionDecoder::getOpcode(instr1) !=
                InstructionDecoder::OpType::CMP ||
            InstructionDecoder::getOpcode(instr2) !=
                InstructionDecoder::OpType::CMP) {
          return {false, 0};
        }

        // If comparing the same register with the same value, the second is
        // redundant
        if (InstructionDecoder::usesImmediate(instr1) &&
            InstructionDecoder::usesImmediate(instr2)) {
          if (InstructionDecoder::getSrcReg1(instr1) ==
                  InstructionDecoder::getSrcReg1(instr2) &&
              InstructionDecoder::getImmediate(instr1) ==
                  InstructionDecoder::getImmediate(instr2)) {
            return {true, 2};
          }
        } else if (!InstructionDecoder::usesImmediate(instr1) &&
                   !InstructionDecoder::usesImmediate(instr2)) {
          if (InstructionDecoder::getSrcReg1(instr1) ==
                  InstructionDecoder::getSrcReg1(instr2) &&
              InstructionDecoder::getSrcReg2(instr1) ==
                  InstructionDecoder::getSrcReg2(instr2)) {
            return {true, 2};
          }
        }

        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        // Keep only one compare instruction
        return {instrs[pos]};
      },
      "Redundant compare elimination");
}

std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createConstantFoldingPattern() {
  // Pattern: mov x0, #c1; add x0, x0, #c2  ->  mov x0, #(c1+c2)
  return std::make_unique<InstructionPattern>(
      2, // Pattern size
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];

        // Check if we have a mov followed by an add with immediate
        if (InstructionDecoder::getOpcode(instr1) !=
                InstructionDecoder::OpType::MOV ||
            InstructionDecoder::getOpcode(instr2) !=
                InstructionDecoder::OpType::ADD) {
          return {false, 0};
        }

        // Both instructions must use immediate values
        if (!InstructionDecoder::usesImmediate(instr1) ||
            !InstructionDecoder::usesImmediate(instr2)) {
          return {false, 0};
        }

        // Check if same register used in both instructions
        // MOV rd, #imm followed by ADD rd, rd, #imm
        if (InstructionDecoder::getDestReg(instr1) ==
                InstructionDecoder::getDestReg(instr2) &&
            InstructionDecoder::getDestReg(instr1) ==
                InstructionDecoder::getSrcReg1(instr2)) {
          return {true, 2};
        }
        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        // Extract values and compute the new constant
        const auto &mov_instr = instrs[pos];
        const auto &add_instr = instrs[pos + 1];

        int reg_num = mov_instr.dest_reg;
        int64_t val1 = mov_instr.immediate;
        int64_t val2 = add_instr.immediate;
        int64_t result = val1 + val2;

        // Only allow folding if result fits in a single MOVZ (16-bit unsigned)
        if (result < 0 || result > 65535) {
          return {mov_instr, add_instr};
        }

        // Convert the integer register number to its string name first.
        std::string reg_name = InstructionDecoder::getRegisterName(reg_num);

        // Use the existing Encoder function for MOVZ with the correct string
        // type
        Instruction new_instr = Encoder::create_movz_imm(reg_name, result);

        return {new_instr};
      },
      "Constant folding (add)");
}

std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createStrengthReductionPattern() {
  // Pattern: mul x0, x1, #2  ->  add x0, x1, x1
  return std::make_unique<InstructionPattern>(
      1, // Pattern size
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        const auto &instr = instrs[pos];

        // Check if we have a multiply by 2 using semantic information
        if (InstructionDecoder::getOpcode(instr) ==
                InstructionDecoder::OpType::MUL &&
            InstructionDecoder::usesImmediate(instr) &&
            InstructionDecoder::getImmediate(instr) == 2) {
          return {true, 1};
        }
        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        const auto &instr = instrs[pos];
        int dest_reg = instr.dest_reg;
        int src_reg = instr.src_reg1;

        // Convert integer IDs back to string names locally
        std::string dest_reg_name =
            InstructionDecoder::getRegisterName(dest_reg);
        std::string src_reg_name = InstructionDecoder::getRegisterName(src_reg);

        // Call the existing Encoder function with the expected string types
        Instruction new_instr =
            Encoder::create_add_reg(dest_reg_name, src_reg_name, src_reg_name);

        return {new_instr};
      },
      "Multiply by 2 strength reduction");
}

//
std::unique_ptr<InstructionPattern>
PeepholeOptimizer::createMovzMovFusionPattern() {
  return std::make_unique<InstructionPattern>(
      2, // Pattern size
      [](const std::vector<Instruction> &instrs, size_t pos) -> MatchResult {
        if (pos + 1 >= instrs.size())
          return {false, 0};
        const auto &instr1 = instrs[pos];
        const auto &instr2 = instrs[pos + 1];

        // Match: MOVZ Wd/Xd, #imm followed by MOV Xn, Wd/Xd
        if (instr1.opcode == InstructionDecoder::OpType::MOVZ &&
            instr2.opcode == InstructionDecoder::OpType::MOV &&
            InstructionComparator::areSameRegister(instr1.dest_reg,
                                                   instr2.src_reg1)) {
          // Liveness check: Ensure the intermediate register isn't used later
          for (size_t i = pos + 2; i < instrs.size(); ++i) {
            if (InstructionComparator::areSameRegister(instrs[i].dest_reg,
                                                       instr1.dest_reg))
              break; // Redefined
            if (InstructionComparator::areSameRegister(instrs[i].src_reg1,
                                                       instr1.dest_reg) ||
                InstructionComparator::areSameRegister(instrs[i].src_reg2,
                                                       instr1.dest_reg)) {
              return {false, 0}; // Used later, cannot optimize
            }
          }
          return {true, 2};
        }
        return {false, 0};
      },
      [](const std::vector<Instruction> &instrs,
         size_t pos) -> std::vector<Instruction> {
        const auto &movz_instr = instrs[pos];
        const auto &mov_instr = instrs[pos + 1];

        // Create a new MOVZ that writes directly to the final destination
        // register
        return {Encoder::create_movz_imm(
            InstructionDecoder::getDestRegAsString(mov_instr), // Corrected line
            movz_instr.immediate,
            0 // Assuming shift is 0
            )};
      },
      "Fuse MOVZ with subsequent MOV");
}


std::unique_ptr<InstructionPattern> PeepholeOptimizer::createRedundantIndexCalculationPattern() {
    return std::make_unique<InstructionPattern>(
        3, // This pattern looks at a window of 3 instructions.
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            // 1. Boundary Check: Ensure there are enough instructions for the pattern.
            if (pos + 2 >= instrs.size()) {
                return {false, 0};
            }

            const auto& instr1 = instrs[pos];     // Potential MOVZ reg, #0
            const auto& instr2 = instrs[pos + 1]; // Potential LSL reg, reg, #imm
            const auto& instr3 = instrs[pos + 2]; // Potential ADD dest, base, reg

            // 2. Opcode Check: Verify the instruction types.
            if (instr1.opcode != InstructionDecoder::OpType::MOVZ ||
                instr2.opcode != InstructionDecoder::OpType::LSL  ||
                instr3.opcode != InstructionDecoder::OpType::ADD) {
                return {false, 0};
            }

            // 3. Immediate Check: Ensure the MOVZ is loading zero.
            if (instr1.immediate != 0) {
                return {false, 0};
            }

            // 4. Register Flow Check: Verify that the same register is used throughout the sequence.
            //    - The destination of MOVZ is the source and destination of LSL.
            //    - The destination of LSL is the second source of ADD.
            if (InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr3.src_reg2)) {
                return {true, 3}; // Match found!
            }

            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // The transformer lambda is simple: it replaces the 3-instruction sequence
            // with a single MOV instruction.

            const auto& add_instr = instrs[pos + 2]; // The final ADD instruction

            // The new MOV's destination is the destination of the original ADD.
            int dest_reg_num = InstructionDecoder::getDestReg(add_instr);

            // The new MOV's source is the base register (the first source) of the original ADD.
            int base_reg_num = InstructionDecoder::getSrcReg1(add_instr);

            // Create the new, optimized instruction.
            Instruction new_mov = Encoder::create_mov_reg(
                InstructionDecoder::getRegisterName(dest_reg_num),
                InstructionDecoder::getRegisterName(base_reg_num)
            );

            return { new_mov }; // Return a vector containing only the new instruction.
        },
        "Redundant Index Calculation (add zero)"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createLoadStoreForwardingPatternSimple() {
    return std::make_unique<InstructionPattern>(
        2, // Fixed pattern size of 2 instructions
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            // 1. Boundary Check
            if (pos + 1 >= instrs.size()) {
                return {false, 0};
            }

            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // 2. Opcode Check: Must be STR followed immediately by LDR.
            if (instr1.opcode != InstructionDecoder::OpType::STR ||
                instr2.opcode != InstructionDecoder::OpType::LDR) {
                return {false, 0};
            }

            // 3. Memory Operand Check: Use the comparator to see if they access the same address.
            if (!InstructionComparator::haveSameMemoryOperand(instr1, instr2)) {
                return {false, 0};
            }

            // 4. Safety Check: Don't optimize if the source and destination are the same register,
            // as this would result in a redundant "MOV x0, x0".
            if (InstructionComparator::areSameRegister(instr1.src_reg1, instr2.dest_reg)) {
                return {false, 0};
            }

            return {true, 2}; // Match found!
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& store_instr = instrs[pos];
            const auto& load_instr = instrs[pos + 1];

            // Create the new MOV instruction.
            // The destination is the original LDR's destination.
            // The source is the original STR's source.
            Instruction new_mov = Encoder::create_mov_reg(
                InstructionDecoder::getDestRegAsString(load_instr),
                InstructionDecoder::getSrcReg1AsString(store_instr)
            );

            // Return the original store followed by the new, faster MOV.
            return { store_instr, new_mov };
        },
        "Load-Store Forwarding (Simple)"
    );
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createAdjacentLoadStoreForwardingPattern() {
    return std::make_unique<InstructionPattern>(
        2, // Fixed pattern size of 2
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& instr1_store = instrs[pos];
            const auto& instr2_load = instrs[pos + 1];

            // Match: STR followed immediately by LDR
            if (instr1_store.opcode != InstructionDecoder::OpType::STR ||
                instr2_load.opcode != InstructionDecoder::OpType::LDR) {
                return {false, 0};
            }

            // Match: They access the exact same memory location.
            if (!InstructionComparator::haveSameMemoryOperand(instr1_store, instr2_load)) {
                return {false, 0};
            }

            return {true, 2};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& store_instr = instrs[pos];
            const auto& load_instr = instrs[pos + 1];

            // Replace the LDR with MOV: dest_reg <- src_reg
            Instruction new_mov = Encoder::create_mov_reg(
                InstructionDecoder::getDestRegAsString(load_instr),
                InstructionDecoder::getSrcReg1AsString(store_instr)
            );

            // Keep the original store, but replace the load with the move.
            return { store_instr, new_mov };
        },
        "Load-Store Forwarding (Adjacent)"
    );
}
