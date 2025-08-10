#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <cmath>
#include <iostream>
#include <algorithm>
#include <regex>

/**
 * @brief Creates a pattern to fuse floating-point multiply and add/subtract instructions.
 * Pattern 1: FMUL Vd, Vn, Vm + FADD Vd, Vd, Vz -> FMADD Vd, Vn, Vm, Vz
 * Pattern 2: FMUL Vd, Vn, Vm + FSUB Vd, Vd, Vz -> FMSUB Vd, Vn, Vm, Vz
 * 
 * This optimization reduces instruction count and takes advantage of the ARM64
 * hardware's ability to perform a fused multiply-add in a single instruction.
 * 
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createFusedMultiplyAddPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: 2 instructions (FMUL + FADD/FSUB)
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            // Check if we have enough instructions
            if (pos + 1 >= instrs.size()) return false;

            const auto& mul_instr = instrs[pos];
            const auto& add_instr = instrs[pos + 1];

            // Check instruction opcodes directly.
            if (mul_instr.opcode != InstructionDecoder::OpType::FMUL) return false;
            bool is_add = (add_instr.opcode == InstructionDecoder::OpType::FADD);
            bool is_sub = (add_instr.opcode == InstructionDecoder::OpType::FSUB);
            if (!is_add && !is_sub) return false;

            // Use semantic register numbers for comparison.
            // Pattern: FMUL Vd, Vn, Vm followed by FADD/FSUB Vd, Vd, Vz
            bool dest_match = InstructionComparator::areSameRegister(mul_instr.dest_reg, add_instr.dest_reg);
            bool src_match = InstructionComparator::areSameRegister(mul_instr.dest_reg, add_instr.src_reg1);

            return dest_match && src_match;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& mul_instr = instrs[pos];
            const auto& add_instr = instrs[pos + 1];

            int vd = mul_instr.dest_reg;
            int vn = mul_instr.src_reg1;
            int vm = mul_instr.src_reg2;
            int vz = add_instr.src_reg2;

            if (add_instr.opcode == InstructionDecoder::OpType::FADD) {
                Instruction fmadd_instr = EncoderExtended::create_fmadd(vd, vn, vm, vz);
                return { fmadd_instr };
            } else { // FSUB
                Instruction fmsub_instr = EncoderExtended::create_fmsub(vd, vn, vm, vz);
                return { fmsub_instr };
            }
        },
        "Floating-point multiply-add fusion"
    );
}