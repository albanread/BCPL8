#ifndef PEEPHOLE_OPTIMIZER_H
#define PEEPHOLE_OPTIMIZER_H

#include "InstructionStream.h"
#include "Encoder.h"
#include "LabelManager.h"
#include "EncoderExtended.h"
#include <vector>
#include <string>
#include <functional>
#include <array>
#include <unordered_map>
#include <memory>

// Struct to hold match information for patterns
struct MatchResult {
    bool matched = false;
    size_t length = 0; // The actual number of instructions matched
};

// Pattern matching class for peephole optimization
class InstructionPattern {
public:
    using MatcherFunction = std::function<MatchResult(const std::vector<Instruction>&, size_t)>;

    InstructionPattern(size_t pattern_size, MatcherFunction matcher_func, 
                      std::function<std::vector<Instruction>(const std::vector<Instruction>&, size_t)> transformer_func,
                      std::string description);

    // Check if pattern matches at the given position in the instruction stream
    MatchResult matches(const std::vector<Instruction>& instructions, size_t position) const;
    
    // Transform matched instructions into optimized form
    std::vector<Instruction> transform(const std::vector<Instruction>& instructions, size_t position) const;
    
    // Get the number of instructions this pattern matches (default/fallback for fixed-size patterns)
    size_t getSize() const { return pattern_size_; }
    
    // Get the description of this optimization pattern
    const std::string& getDescription() const { return description_; }

private:
    size_t pattern_size_;
    MatcherFunction matcher_;
    std::function<std::vector<Instruction>(const std::vector<Instruction>&, size_t)> transformer_;
    std::string description_;
};

// Main peephole optimizer class
class PeepholeOptimizer {
public:
    PeepholeOptimizer(bool enable_tracing = false);
    
    // Apply optimizations to the instruction stream
    // @param instruction_stream The stream of instructions to optimize
    // @param max_passes Maximum number of optimization passes to run (default: 5)
    void optimize(InstructionStream& instruction_stream, int max_passes = 5);
    
    // Add a new optimization pattern
    void addPattern(std::unique_ptr<InstructionPattern> pattern);
    
    // Get statistics about optimizations applied
    struct OptimizationStats {
        int total_instructions_before;
        int total_instructions_after;
        int optimizations_applied;
        std::unordered_map<std::string, int> pattern_matches;
        
        void clear() {
            total_instructions_before = 0;
            total_instructions_after = 0;
            optimizations_applied = 0;
            pattern_matches.clear();
        }
    };
    
    const OptimizationStats& getStats() const { return stats_; }
    
    // Factory methods for common patterns
    static std::unique_ptr<InstructionPattern> createRedundantMovePattern();
    static std::unique_ptr<InstructionPattern> createLoadAfterStorePattern();
    static std::unique_ptr<InstructionPattern> createDeadStorePattern();
    static std::unique_ptr<InstructionPattern> createRedundantComparePattern();
    static std::unique_ptr<InstructionPattern> createConstantFoldingPattern();
    static std::unique_ptr<InstructionPattern> createStrengthReductionPattern();
    
    // New advanced optimization patterns
    static std::unique_ptr<InstructionPattern> createMultiplyByPowerOfTwoPattern();
    static std::unique_ptr<InstructionPattern> createDivideByPowerOfTwoPattern();
    static std::unique_ptr<InstructionPattern> createCompareZeroBranchPattern();
    static std::unique_ptr<InstructionPattern> createFuseAluOperationsPattern();
    static std::unique_ptr<InstructionPattern> createLoadStoreForwardingPattern();
    static std::unique_ptr<InstructionPattern> createFusedMultiplyAddPattern();
    static std::unique_ptr<InstructionPattern> createConditionalSelectPattern();
    static std::unique_ptr<InstructionPattern> createBitFieldOperationsPattern();
    static std::unique_ptr<InstructionPattern> createAddressGenerationPattern();

    static std::unique_ptr<InstructionPattern> createIdentityOperationEliminationPattern();
    static std::unique_ptr<InstructionPattern> createFuseMOVAndALUPattern();
    static std::unique_ptr<InstructionPattern> createBranchChainingPattern();
    static std::unique_ptr<InstructionPattern> createRedundantLoadEliminationPattern();
    
private:
    std::vector<std::unique_ptr<InstructionPattern>> patterns_;
    OptimizationStats stats_;
    bool enable_tracing_;
    
    // Helper method to apply a single pass of optimization
    bool applyOptimizationPass(std::vector<Instruction>& instructions);
    
    // Helper method to trace optimizations when enabled
    void trace(const std::string& message) const;
    
    // Helper method to trace optimization details with before/after comparison
    void traceOptimization(const std::string& description, 
                          const std::vector<Instruction>& before,
                          const std::vector<Instruction>& after,
                          size_t position) const;
    
    // Check if an instruction is a label or other special instruction that should not be modified
    static bool isSpecialInstruction(const Instruction& instr);
    
    // Check if an optimization would affect label references
    bool wouldBreakLabelReferences(const std::vector<Instruction>& instructions, 
                                  size_t start_pos, size_t count, 
                                  const std::vector<Instruction>& replacements) const;

    // Declaration for copy propagation pattern
    static std::unique_ptr<InstructionPattern> createCopyPropagationPattern();
};

#endif // PEEPHOLE_OPTIMIZER_H