#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include <iostream>
#include <algorithm>

/**
 * @brief Apply a single optimization pass to the instruction stream
 * 
 * @param instructions The instruction stream to optimize
 * @return true if any optimizations were applied
 * @return false if no optimizations were applied
 */
bool PeepholeOptimizer::applyOptimizationPass(std::vector<Instruction>& instructions) {
    bool any_changes = false;
    size_t pos = 0;

    while (pos < instructions.size()) {
        bool applied_optimization = false;

        if (isSpecialInstruction(instructions[pos])) {
            pos++;
            continue;
        }

        for (const auto& pattern : patterns_) {
            // Modern: use MatchResult struct for variable-length patterns
            MatchResult result = pattern->matches(instructions, pos);

            if (result.matched) {
                // Optionally: check for label reference safety here if needed

                std::vector<Instruction> replacements = pattern->transform(instructions, pos);

                if (enable_tracing_) {
                    std::vector<Instruction> original_instructions(
                        instructions.begin() + pos, instructions.begin() + pos + result.length);
                    traceOptimization(pattern->getDescription(), original_instructions, replacements, pos);
                }

                instructions.erase(instructions.begin() + pos,
                                  instructions.begin() + pos + result.length);
                instructions.insert(instructions.begin() + pos,
                                   replacements.begin(), replacements.end());

                stats_.optimizations_applied++;
                stats_.pattern_matches[pattern->getDescription()]++;

                any_changes = true;
                applied_optimization = true;
                break; // Restart scan from the same position
            }
        }

        if (!applied_optimization) {
            pos++;
        }
    }
    return any_changes;
}