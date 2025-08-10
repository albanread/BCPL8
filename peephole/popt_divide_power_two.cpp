#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <cmath>
#include <iostream>
#include <algorithm>

/**
 * @brief Creates a pattern to recognize division by powers of two and convert to right shifts.
 * Pattern: SDIV Xd, Xn, #power_of_two -> ASR Xd, Xn, #log2(power_of_two)
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createDivideByPowerOfTwoPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: needs 2 instructions (MOVZ + SDIV)
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            // Check if we have at least 2 instructions
            if (pos + 1 >= instrs.size()) return false;
            
            const auto& mov_instr = instrs[pos];
            const auto& div_instr = instrs[pos + 1];
            
            // Check if first instruction loads a constant (MOVZ)
            if (InstructionDecoder::getOpcode(mov_instr) != InstructionDecoder::OpType::MOV ||
                !InstructionDecoder::usesImmediate(mov_instr)) {
                return false;
            }
            
            // Get the immediate value from the MOV
            int64_t immediate = InstructionDecoder::getImmediate(mov_instr);
            
            // Check if immediate is a power of two
            if (!EncoderExtended::isPowerOfTwo(immediate)) {
                return false;
            }
            
            // Check if second instruction is a divide
            if (InstructionDecoder::getOpcode(div_instr) != InstructionDecoder::OpType::DIV) {
                return false;
            }
            
            // Use integer register numbers for robust comparison
            int mov_dest_reg = mov_instr.dest_reg;
            int div_dest_reg = div_instr.dest_reg;
            int div_src1_reg = div_instr.src_reg1;
            int div_src2_reg = div_instr.src_reg2;

            // Check if the register with the constant is used as a source in the divide
            return InstructionComparator::areSameRegister(mov_dest_reg, div_src2_reg) &&
                   !InstructionComparator::areSameRegister(mov_dest_reg, div_dest_reg) &&
                   !InstructionComparator::areSameRegister(mov_dest_reg, div_src1_reg);
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& mov_instr = instrs[pos];
            const auto& div_instr = instrs[pos + 1];
            
            // Get the immediate value from the MOV
            int64_t immediate = InstructionDecoder::getImmediate(mov_instr);
            
            // Calculate shift amount (log base 2)
            int shift_amount = EncoderExtended::log2OfPowerOfTwo(immediate);
            
            // Use integer register numbers for destination and source
            int dest_reg_num = div_instr.dest_reg;
            int src_reg_num = div_instr.src_reg1;

            // Create ASR instruction with proper binary encoding
            Instruction asr_instr = EncoderExtended::create_asr_imm(dest_reg_num, src_reg_num, shift_amount);

            return { asr_instr };
        },
        "Division by power of two converted to arithmetic shift"
    );
}