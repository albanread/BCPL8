#include "../PeepholePatterns.h"
#include "../../Encoder.h"
#include "../../InstructionDecoder.h"
#include <memory>
#include <cmath>

namespace PeepholePatterns {

// Strength Reduction: Replace MUL by power of two with LSL
std::unique_ptr<InstructionPattern> createMultiplyByPowerOfTwoPattern() {
    return std::make_unique<InstructionPattern>(
        1,  // Pattern size: 1 instruction (MUL or MADD acting as MUL)
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];

            // Check if the instruction is semantically a MUL
            if (InstructionDecoder::getOpcode(instr) != InstructionDecoder::OpType::MUL) {
                return {false, 0};
            }

            // We need to check if the value in Rm is a power of two.
            // This is only possible if the multiplier is an immediate (not a register).
            // If it's a register, we can't do this optimization here.
            // For now, this pattern is a placeholder and will not match.
            // In a more advanced optimizer, you could track constant values in registers.
            return {false, 0}; // Disabled until constant propagation is available
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Placeholder: return the original instruction
            return { instrs[pos] };
        },
        "Multiply by power of two converted to shift"
    );
}

// Strength Reduction: Replace DIV by power of two with LSR
std::unique_ptr<InstructionPattern> createDivideByPowerOfTwoPattern() {
    return std::make_unique<InstructionPattern>(
        1,  // Pattern size: 1 instruction (SDIV)
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];

            if (InstructionDecoder::getOpcode(instr) != InstructionDecoder::OpType::SDIV) {
                return {false, 0};
            }

            // As above, we would need to know if the divisor is a power of two constant.
            // This is not possible without constant propagation.
            return {false, 0}; // Disabled until constant propagation is available
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Placeholder: return the original instruction
            return { instrs[pos] };
        },
        "Divide by power of two converted to shift"
    );
}

// General Strength Reduction: Replace expensive operations with cheaper ones (future expansion)
std::unique_ptr<InstructionPattern> createStrengthReductionPattern() {
    // This is a placeholder for more general strength reduction patterns.
    return std::make_unique<InstructionPattern>(
        1,
        [](const std::vector<Instruction>&, size_t) -> MatchResult {
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            return { instrs[pos] };
        },
        "General strength reduction (placeholder)"
    );
}

} // namespace PeepholePatterns