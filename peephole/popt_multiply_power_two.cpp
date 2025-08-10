#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <cmath>
#include <iostream>
#include <algorithm>

/**
 * @brief Creates a pattern to recognize multiplication by powers of two and convert to left shifts.
 * Pattern: MUL Xd, Xn, #power_of_two -> LSL Xd, Xn, #log2(power_of_two)
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createMultiplyByPowerOfTwoPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: needs 2 instructions (MOVZ + MUL)
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            // Check if we have at least 2 instructions
            if (pos + 1 >= instrs.size()) return false;
            
            const auto& mov_instr = instrs[pos];
            const auto& mul_instr = instrs[pos + 1];
            
            // Check if first instruction loads a constant (MOVZ)
            // We can extend this later to also match MOV with a register that has a known value
            if (InstructionDecoder::getOpcode(mov_instr) != InstructionDecoder::OpType::MOV ||
                !InstructionDecoder::usesImmediate(mov_instr)) {
                return false;
            }
            
            // Get the immediate value from the MOV
            int64_t immediate = InstructionDecoder::getImmediate(mov_instr);
            
            // Check if immediate is a power of two (excluding 1 and 2, as they're already optimized)
            if (!EncoderExtended::isPowerOfTwo(immediate) || immediate <= 2) {
                return false;
            }
            
            // Check if second instruction is a multiply
            if (InstructionDecoder::getOpcode(mul_instr) != InstructionDecoder::OpType::MUL) {
                return false;
            }
            
            // Use integer register numbers for robust comparison
            int mov_dest_reg = mov_instr.dest_reg;
            int mul_dest_reg = mul_instr.dest_reg;
            int mul_src1_reg = mul_instr.src_reg1;
            int mul_src2_reg = mul_instr.src_reg2;

            // Check if the register with the constant is used as a source in the multiply
            // and doesn't interfere with other registers.
            return InstructionComparator::areSameRegister(mov_dest_reg, mul_src2_reg) &&
                   !InstructionComparator::areSameRegister(mov_dest_reg, mul_dest_reg) &&
                   !InstructionComparator::areSameRegister(mov_dest_reg, mul_src1_reg);
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& mov_instr = instrs[pos];
            const auto& mul_instr = instrs[pos + 1];
            
            // Get the immediate value from the MOV
            int64_t immediate = InstructionDecoder::getImmediate(mov_instr);
            
            // Calculate shift amount (log base 2)
            int shift_amount = EncoderExtended::log2OfPowerOfTwo(immediate);
            
            // Use integer register numbers for destination and source
            int dest_reg_num = mul_instr.dest_reg;
            int src_reg_num = mul_instr.src_reg1;

            // Create LSL instruction with proper binary encoding
            Instruction lsl_instr = EncoderExtended::create_lsl_imm(dest_reg_num, src_reg_num, shift_amount);

            return { lsl_instr };
        },
        "Multiply by power of two converted to shift"
    );
}