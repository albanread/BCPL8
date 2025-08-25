#include "../PeepholePatterns.h"
#include "../../Encoder.h"
#include "../../InstructionDecoder.h"
#include <memory>
#include <string>

namespace PeepholePatterns {

// ADRP/ADD to ADR Fusion Pattern
std::unique_ptr<InstructionPattern> createAdrFusionPattern() {
    return std::make_unique<InstructionPattern>(
        2, // Pattern size: ADRP followed by ADD
        [](const std::vector<Instruction>& instrs, size_t pos) -> MatchResult {
            if (pos + 1 >= instrs.size()) return {false, 0};
            const auto& adrp_instr = instrs[pos];
            const auto& add_instr = instrs[pos + 1];

            // Match ADRP followed by ADD to the same register, with the same label
            if (InstructionDecoder::getOpcode(adrp_instr) != InstructionDecoder::OpType::ADRP)
                return {false, 0};
            if (InstructionDecoder::getOpcode(add_instr) != InstructionDecoder::OpType::ADD)
                return {false, 0};
            // Both must target the same register
            if (adrp_instr.dest_reg != add_instr.dest_reg)
                return {false, 0};
            // ADD must use the result of ADRP as its first source
            if (add_instr.src_reg1 != adrp_instr.dest_reg)
                return {false, 0};
            // ADD must add the same label as ADRP
            if (adrp_instr.target_label.empty() || add_instr.target_label.empty())
                return {false, 0};
            if (adrp_instr.target_label != add_instr.target_label)
                return {false, 0};
            return {true, 2};
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& adrp_instr = instrs[pos];
            // The actual fusion logic requires label addresses, which are only available after linking.
            // For now, we emit a single ADR instruction with a relocation, and let the linker patch it.
            Instruction adr_instr = Encoder::create_adr(
                InstructionDecoder::getRegisterName(adrp_instr.dest_reg),
                adrp_instr.target_label
            );
            return { adr_instr };
        },
        "ADR fusion (ADRP+ADD to ADR)"
    );
}

} // namespace PeepholePatterns