#include "../PeepholePatterns.h"
#include "../../Encoder.h"
#include "../../InstructionDecoder.h"
#include <memory>

namespace PeepholePatterns {

// Constant Folding Pattern: Fold ADD/SUB/MUL/AND/ORR/EOR with two immediate operands
std::unique_ptr<InstructionPattern> createConstantFoldingPattern() {
    return std::make_unique<InstructionPattern>(
        1, // Pattern size: 1 instruction (could be expanded for multi-instruction folding)
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];
            // Only fold if both operands are immediate (rare in real code, but possible in generated code)
            // For now, this is a placeholder and will not match.
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Placeholder: return the original instruction
            return { instrs[pos] };
        },
        "Constant folding (placeholder)"
    );
}

// Identity Operation Elimination: Remove ADD Xd, Xn, #0 or SUB Xd, Xn, #0
std::unique_ptr<InstructionPattern> createIdentityOperationEliminationPattern() {
    return std::make_unique<InstructionPattern>(
        1,
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];
            auto op = InstructionDecoder::getOpcode(instr);
            if ((op == InstructionDecoder::OpType::ADD || op == InstructionDecoder::OpType::SUB) &&
                InstructionDecoder::usesImmediate(instr) &&
                InstructionDecoder::getImmediate(instr) == 0) {
                return {true, 1};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // Replace with a MOV Xd, Xn
            const auto& instr = instrs[pos];
            std::string xd = InstructionDecoder::getRegisterName(instr.dest_reg);
            std::string xn = InstructionDecoder::getRegisterName(instr.src_reg1);
            Instruction mov = Encoder::create_mov_reg(xd, xn);
            return { mov };
        },
        "Identity operation elimination (ADD/SUB #0 to MOV)"
    );
}

// Redundant Move Pattern: Remove MOV Xd, Xn when Xd == Xn
std::unique_ptr<InstructionPattern> createRedundantMovePattern() {
    return std::make_unique<InstructionPattern>(
        1,
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];
            if (InstructionDecoder::getOpcode(instr) == InstructionDecoder::OpType::MOV &&
                instr.dest_reg == instr.src_reg1) {
                return {true, 1};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>&, size_t) -> std::vector<Instruction> {
            // Remove the redundant MOV
            return {};
        },
        "Redundant move elimination (MOV Xn, Xn)"
    );
}

// Self Move Elimination: Remove MOV Xn, Xn (alias for above, but kept for clarity)
std::unique_ptr<InstructionPattern> createSelfMoveEliminationPattern() {
    return std::make_unique<InstructionPattern>(
        1,
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            const auto& instr = instrs[pos];
            if (InstructionDecoder::getOpcode(instr) == InstructionDecoder::OpType::MOV &&
                instr.dest_reg == instr.src_reg1) {
                return {true, 1};
            }
            return {false, 0};
        },
        [](const std::vector<Instruction>&, size_t) -> std::vector<Instruction> {
            // Remove the self-move
            return {};
        },
        "Self move elimination (MOV Xn, Xn)"
    );
}

} // namespace PeepholePatterns