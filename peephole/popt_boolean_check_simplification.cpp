NewBCPL/peephole/popt_boolean_check_simplification.cpp#L1-54
#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"

// Helper function to invert a condition code.
static ConditionCode invertCondition(ConditionCode cond) {
    switch (cond) {
        case ConditionCode::EQ: return ConditionCode::NE;
        case ConditionCode::NE: return ConditionCode::EQ;
        case ConditionCode::CS: return ConditionCode::CC;
        case ConditionCode::CC: return ConditionCode::CS;
        case ConditionCode::MI: return ConditionCode::PL;
        case ConditionCode::PL: return ConditionCode::MI;
        case ConditionCode::VS: return ConditionCode::VC;
        case ConditionCode::VC: return ConditionCode::VS;
        case ConditionCode::HI: return ConditionCode::LS;
        case ConditionCode::LS: return ConditionCode::HI;
        case ConditionCode::GE: return ConditionCode::LT;
        case ConditionCode::LT: return ConditionCode::GE;
        case ConditionCode::GT: return ConditionCode::LE;
        case ConditionCode::LE: return ConditionCode::GT;
        default: return ConditionCode::UNKNOWN;
    }
}

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createBooleanCheckSimplificationPattern() {
    return std::make_unique<InstructionPattern>(
        4, // This is a fixed-window pattern of 4 instructions.
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            // --- MATCHER LOGIC ---
            if (pos + 3 >= instrs.size()) return {false, 0};

            const auto& instr1_cmp = instrs[pos];
            const auto& instr2_cset = instrs[pos + 1];
            const auto& instr3_cmp_zr = instrs[pos + 2];
            const auto& instr4_branch = instrs[pos + 3];

            // 1. Check Opcodes: Must be CMP, CSET, CMP, B.cond
            if (instr1_cmp.opcode != InstructionDecoder::OpType::CMP ||
                instr2_cset.opcode != InstructionDecoder::OpType::CSET ||
                instr3_cmp_zr.opcode != InstructionDecoder::OpType::CMP ||
                instr4_branch.opcode != InstructionDecoder::OpType::B_COND) {
                return {false, 0};
            }

            // 2. Check Register Flow: The result of CSET must be the input to the second CMP.
            if (!InstructionComparator::areSameRegister(instr2_cset.dest_reg, instr3_cmp_zr.src_reg1)) {
                return {false, 0};
            }

            // 3. Check Second Compare: Must be a compare against the zero register (XZR).
            if (instr3_cmp_zr.src_reg2 != 31) { // 31 is the encoding for XZR/WZR
                return {false, 0};
            }

            // All checks passed. This is a valid pattern.
            return {true, 4};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            // --- TRANSFORMER LOGIC ---
            // Note: The matcher guarantees pos+3 is valid.
            const auto& instr1_cmp = instrs[pos];
            const auto& instr4_branch = instrs[pos + 3];

            // The first new instruction is simply a copy of the original CMP.
            Instruction new_cmp = instr1_cmp;

            // The second new instruction is a conditional branch with the INVERSE condition.
            ConditionCode final_branch_cond = instr4_branch.cond;
            ConditionCode inverted_cond = invertCondition(final_branch_cond);

            // This will require an Encoder function that takes a condition code and a label.
            // If it doesn't exist, it must be added.
            Instruction new_branch = Encoder::create_branch_conditional(
                inverted_cond,
                instr4_branch.target_label
            );

            return { new_cmp, new_branch };
        },
        "Boolean Check Simplification"
    );
}