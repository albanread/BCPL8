#include "../PeepholePatterns.h"
#include "../../Encoder.h"
#include "../../InstructionDecoder.h"
#include "../../InstructionComparator.h"

namespace PeepholePatterns {

// Redundant Load Elimination Pattern: LDR Rd, [..]; LDR Rd, [..] => LDR Rd, [..]
std::unique_ptr<InstructionPattern> createRedundantLoadEliminationPattern() {
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
            return {instr1, new_mov};
        },
        "Redundant load elimination (LDR Rd, [..]; LDR Rd, [..] => LDR Rd, [..])"
    );
}

std::unique_ptr<InstructionPattern> createLoadAfterStorePattern() {
    return std::make_unique<InstructionPattern>(
        2, // Pattern size: 2 instructions (STR followed by LDR)
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& str_instr = instrs[pos];
            const auto& ldr_instr = instrs[pos + 1];

            // STR followed by LDR from the same address
            if (str_instr.opcode != InstructionDecoder::OpType::STR ||
                ldr_instr.opcode != InstructionDecoder::OpType::LDR) {
                return {false, 0};
            }
            // Must be same base register and offset
            if (!InstructionComparator::areSameRegister(str_instr.base_reg, ldr_instr.base_reg) ||
                str_instr.immediate != ldr_instr.immediate) {
                return {false, 0};
            }
            // STR's src_reg must match LDR's dest_reg
            if (!InstructionComparator::areSameRegister(str_instr.src_reg1, ldr_instr.dest_reg)) {
                return {false, 0};
            }
            return {true, 2};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Remove the redundant LDR, keep only the STR
            return {instrs[pos]};
        },
        "Load-after-store elimination (STR+LDR to STR)"
    );
}

std::unique_ptr<InstructionPattern> createDeadStorePattern() {
    return std::make_unique<InstructionPattern>(
        2, // Pattern size: 2 instructions (STR followed by STR to same address)
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& str1 = instrs[pos];
            const auto& str2 = instrs[pos + 1];

            if (str1.opcode != InstructionDecoder::OpType::STR ||
                str2.opcode != InstructionDecoder::OpType::STR) {
                return {false, 0};
            }
            // Must be same base register and offset
            if (!InstructionComparator::areSameRegister(str1.base_reg, str2.base_reg) ||
                str1.immediate != str2.immediate) {
                return {false, 0};
            }
            // The second STR overwrites the first, so the first is dead
            return {true, 2};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Keep only the second STR
            return {instrs[pos + 1]};
        },
        "Dead store elimination (STR+STR to STR)"
    );
}

std::unique_ptr<InstructionPattern> createRedundantStorePattern() {
    return std::make_unique<InstructionPattern>(
        2, // Pattern size: 2 instructions (STR followed by STR to same address)
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& str1 = instrs[pos];
            const auto& str2 = instrs[pos + 1];

            if (str1.opcode != InstructionDecoder::OpType::STR ||
                str2.opcode != InstructionDecoder::OpType::STR) {
                return {false, 0};
            }
            // Must be same base register and offset
            if (!InstructionComparator::areSameRegister(str1.base_reg, str2.base_reg) ||
                str1.immediate != str2.immediate) {
                return {false, 0};
            }
            // If both STRs store the same value, the first is redundant
            if (InstructionComparator::areSameRegister(str1.src_reg1, str2.src_reg1)) {
                return {true, 2};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Keep only one STR (the second)
            return {instrs[pos + 1]};
        },
        "Redundant store elimination (STR+STR to STR)"
    );
}

} // namespace PeepholePatterns