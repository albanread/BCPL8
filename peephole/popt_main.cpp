#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include <cmath>
#include <iostream>
#include <algorithm>

/**
 * @brief Construct a new Peephole Optimizer object
 * 
 * @param enable_tracing Whether to enable detailed tracing of optimizations
 */
PeepholeOptimizer::PeepholeOptimizer(bool enable_tracing)
    : enable_tracing_(enable_tracing) {
    // Add built-in optimization patterns
    addPattern(createRedundantMovePattern());
    
    // --- NOTE ---
    // The "Load after store" pattern has been modified to use a register-to-register MOV
    // instead of removing the load instruction, which would make it safer without liveness analysis.
    // However, it's currently disabled due to potential issues. The implementation is ready for
    // future activation after more thorough testing.
    // addPattern(createLoadAfterStorePattern());
    // --- END NOTE ---
    
    addPattern(createDeadStorePattern());
    addPattern(createRedundantComparePattern());
    addPattern(createConstantFoldingPattern());
    addPattern(createStrengthReductionPattern());
    
    // Add new advanced optimization patterns
    addPattern(createMultiplyByPowerOfTwoPattern());
    addPattern(createDivideByPowerOfTwoPattern());
    addPattern(createCompareZeroBranchPattern());
    addPattern(createFuseAluOperationsPattern());
    addPattern(createLoadStoreForwardingPattern());
    addPattern(createFusedMultiplyAddPattern());
    addPattern(createConditionalSelectPattern());
    addPattern(createBitFieldOperationsPattern());
    addPattern(createAddressGenerationPattern());
    addPattern(createRedundantLoadEliminationPattern());
    addPattern(createIdentityOperationEliminationPattern());
    addPattern(createFuseMOVAndALUPattern());

    // ✅ Add this line
    addPattern(createBooleanCheckSimplificationPattern());
}

/**
 * @brief Add a new optimization pattern to the optimizer
 * 
 * @param pattern Unique pointer to the pattern to add
 */
void PeepholeOptimizer::addPattern(std::unique_ptr<InstructionPattern> pattern) {
    patterns_.push_back(std::move(pattern));
}

/**
 * @brief Apply peephole optimizations to an instruction stream
 * 
 * @param instruction_stream The stream of instructions to optimize
 * @param max_passes Maximum number of optimization passes to run (default: 5)
 */
void PeepholeOptimizer::optimize(InstructionStream& instruction_stream, int max_passes) {
    // Reset statistics
    stats_.clear();

    // Get a copy of the instructions that we can modify
    std::vector<Instruction> instructions = instruction_stream.get_instructions();
    stats_.total_instructions_before = static_cast<int>(instructions.size());

    std::cout << "\n=== Peephole Optimization ===\n";
    std::cout << "Analyzing " << instructions.size() << " ARM64 instructions...\n";
    std::cout << "Maximum optimization passes: " << max_passes << "\n";

    // Apply optimization passes until no more changes are made or we hit max_passes
    int pass_count = 0;
    int pass_count = 0;
    while (pass_count < max_passes) {
        pass_count++;
        bool changes_made = applyOptimizationPass(instructions);

        if (enable_tracing_) {
            trace("Completed pass " + std::to_string(pass_count) + "/" + std::to_string(max_passes) +
                  ", changes made: " + (changes_made ? "yes" : "no"));
        }

        if (!changes_made) {
            trace("No changes in pass " + std::to_string(pass_count) + ". Halting optimization.");
            break;
        }
    }

    // Replace the original instructions with the optimized ones
    instruction_stream.replace_instructions(instructions);

    // Update statistics
    stats_.total_instructions_after = static_cast<int>(instructions.size());

    // Always show optimization results, regardless of trace setting
    std::cout << "Peephole optimization completed " << pass_count << " passes:\n";
    std::cout << "  Passes with changes: " << passes_with_changes << "\n";
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
}

/**
 * @brief Helper method to trace optimizations when enabled
 * 
 * @param message The message to trace
 */
void PeepholeOptimizer::trace(const std::string& message) const {
    if (enable_tracing_) {
        std::cout << "[TRACE] " << message << "\n";
    }
}