#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <iostream>
#include <algorithm>
#include <regex>

/**
 * @brief Creates a pattern for load-store forwarding optimization.
 * 
 * This pattern recognizes when a value is stored to memory with STR, and then
 * loaded back from the same address with LDR, with no intervening stores to that
 * address or modifications to the stored register. It replaces the LDR with a MOV
 * from the original source register, eliminating the unnecessary memory access.
 * 
 * Pattern detected:
 * STR Xs, [Xn, #offset]
 * ... (no interfering STR to [Xn, #offset], and Xs is not modified)
 * LDR Xd, [Xn, #offset]   -->   MOV Xd, Xs
 * 
 * This is known as load-store forwarding.
 * 
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createLoadStoreForwardingPattern() {
    return std::make_unique<InstructionPattern>(
        6, // Pattern size is not strictly used, but window is up to 5
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const int MAX_LOOKAHEAD = 5; // The max distance to search ahead.

            if (pos + 1 >= instrs.size()) return {false, 0};

            const auto& store_instr = instrs[pos];
            if (store_instr.opcode != InstructionDecoder::OpType::STR) {
                return {false, 0};
            }

            // Get the key info from the original STR instruction using its semantic fields.
            int store_src_reg = store_instr.src_reg1;
            int store_base_reg = store_instr.base_reg;
            int64_t store_offset = store_instr.immediate;

            // Scan ahead for a matching LDR.
            for (int i = 1; i <= MAX_LOOKAHEAD && (pos + i) < instrs.size(); ++i) {
                const auto& current_instr = instrs[pos + i];

                // --- Interference Check 1: Memory Interference ---
                // If another instruction writes to our memory location, the pattern is invalid.
                if (current_instr.opcode == InstructionDecoder::OpType::STR || current_instr.opcode == InstructionDecoder::OpType::STP) {
                    if (InstructionComparator::areSameRegister(current_instr.base_reg, store_base_reg) &&
                        current_instr.immediate == store_offset) {
                        return {false, 0}; // Abort: Interfering store found.
                    }
                }

                // --- Interference Check 2: Register Interference ---
                // If the register holding our value (store_src_reg) is modified, the pattern is invalid.
                if (InstructionComparator::areSameRegister(current_instr.dest_reg, store_src_reg)) {
                    return {false, 0}; // Abort: Source register was overwritten.
                }

                // --- Match Check ---
                // If we find a LDR from the same address, it's a match.
                if (current_instr.opcode == InstructionDecoder::OpType::LDR) {
                    bool same_base = InstructionComparator::areSameRegister(current_instr.base_reg, store_base_reg);
                    bool same_offset = (current_instr.immediate == store_offset);

                    if (same_base && same_offset) {
                        // Return a successful match with the DYNAMIC length
                        size_t match_length = (pos + i) - pos + 1;
                        return {true, match_length};
                    }
                }
            }

            return {false, 0}; // No safe match found in the lookahead window.
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& store_instr = instrs[pos];
            const int MAX_LOOKAHEAD = 5;

            // --- First, find the matching LDR again to know the pattern's length ---
            size_t load_idx = 0;
            // This loop must use the same logic as the final part of the matcher.
            for (int i = 1; i <= MAX_LOOKAHEAD && (pos + i) < instrs.size(); ++i) {
                const auto& candidate = instrs[pos + i];
                if (candidate.opcode == InstructionDecoder::OpType::LDR &&
                    InstructionComparator::areSameRegister(candidate.base_reg, store_instr.base_reg) &&
                    candidate.immediate == store_instr.immediate) {
                    load_idx = pos + i;
                    break;
                }
            }

            if (load_idx == 0) return {}; // Safeguard: should not happen

            // --- Now, build the NEW instruction sequence ---
            std::vector<Instruction> result;
            // 1. Copy all instructions from the STR up to (but not including) the LDR.
            for (size_t i = pos; i < load_idx; ++i) {
                result.push_back(instrs[i]);
            }

            // 2. Create the new MOV instruction to replace the LDR.
            int source_reg_num = store_instr.src_reg1;     // Register that was stored.
            int dest_reg_num = instrs[load_idx].dest_reg; // Register that was loaded into.
            Instruction mov_instr = Encoder::create_mov_reg(dest_reg_num, source_reg_num);

            // 3. Add the new MOV to our result sequence.
            result.push_back(mov_instr);

            return result;
        },
            const int MAX_LOOKAHEAD = 5;

            // --- First, find the matching LDR again to know the pattern's length ---
            size_t load_idx = 0;
            for (int i = 1; i <= MAX_LOOKAHEAD && (pos + i) < instrs.size(); ++i) {
                const auto& candidate = instrs[pos + i];
                // Note: This check must be identical to the one in the matcher lambda.
                if (candidate.opcode == InstructionDecoder::OpType::LDR &&
                    InstructionComparator::areSameRegister(candidate.base_reg, store_instr.base_reg) &&
                    candidate.immediate == store_instr.immediate) {
                    load_idx = pos + i;
                    break;
                }
            }
            if (load_idx == 0) {
                // Should not happen if the matcher passed, but as a safeguard:
                return {}; // Return empty to indicate no change
            }

            // --- Now, build the NEW instruction sequence ---
            std::vector<Instruction> result;
            // 1. Copy all the instructions from the STR up to (but not including) the LDR.
            for (size_t i = pos; i < load_idx; ++i) {
                result.push_back(instrs[i]);
            }

            // 2. Create the new MOV instruction to replace the LDR.
            int source_reg_num = store_instr.src_reg1; // Register that was stored
            int dest_reg_num = instrs[load_idx].dest_reg; // Register that was loaded into
            Instruction mov_instr = Encoder::create_mov_reg(dest_reg_num, source_reg_num);

            // 3. Add the new MOV instruction to our result.
            result.push_back(mov_instr);

            // The 'result' vector now contains the optimized sequence of instructions
            // that will replace the original sequence in the main optimizer loop.
            return result;
        },
        "Load-Store Forwarding"
    );
}