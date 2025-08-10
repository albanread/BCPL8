#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include <stdexcept>
#include <iostream>
#include <algorithm>
#include <regex>

/**
 * @brief Creates a pattern to optimize address generation operations.
 * 
 * This pattern recognizes common address calculation sequences and replaces them with
 * more efficient ARM64 addressing modes or instructions.
 * 
 * Patterns detected:
 * 1. ADD Xd, Xn, #imm1; ADD Xd, Xd, Xm -> ADD Xd, Xn, Xm; ADD Xd, Xd, #imm1
 * 2. ADD Xd, Xn, #imm1; ADD Xd, Xd, Xm, LSL #imm2 -> ADD Xd, Xn, Xm, LSL #imm2
 * 3. ADD Xd, Xn, Xm; LDR/STR Xt, [Xd] -> LDR/STR Xt, [Xn, Xm]
 * 4. ADD Xd, Xn, #imm1; LDR/STR Xt, [Xd, #imm2] -> LDR/STR Xt, [Xn, #(imm1+imm2)]
 * 
 * @return A unique pointer to an InstructionPattern.
 */
// We'll use EncoderExtended's encodeRegister method instead of duplicating it here

std::unique_ptr<InstructionPattern> PeepholeOptimizer::createAddressGenerationPattern() {
    return std::make_unique<InstructionPattern>(
        2,  // Pattern size: 2 instructions
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            if (pos + 1 >= instrs.size()) return false;
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Pattern 1: ADD Xd, Xn, #imm; ADD Xd, Xd, Xm
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                instr1.uses_immediate &&
                instr2.opcode == InstructionDecoder::OpType::ADD &&
                !instr2.uses_immediate) {
                // ADD Xd, Xn, #imm; ADD Xd, Xd, Xm
                if (InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg) &&
                    InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1)) {
                    return true;
                }
            }

            // Pattern 2: ADD Xd, Xn, #imm; ADD Xd, Xd, Xm, LSL #imm2
            // (Assume EncoderExtended can recognize and encode shifted register ADD)
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                instr1.uses_immediate &&
                instr2.opcode == InstructionDecoder::OpType::ADD &&
                !instr2.uses_immediate && instr2.shift_type == InstructionDecoder::ShiftType::LSL) {
                if (InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg) &&
                    InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1)) {
                    return true;
                }
            }

            // Pattern 3: ADD Xd, Xn, Xm; LDR/STR Xt, [Xd, #0]
            bool is_load = (instr2.opcode == InstructionDecoder::OpType::LDR);
            bool is_store = (instr2.opcode == InstructionDecoder::OpType::STR);
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                !instr1.uses_immediate &&
                (is_load || is_store) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.base_reg) &&
                instr2.immediate == 0) {
                return true;
            }

            // Pattern 4: ADD Xd, Xn, #imm; LDR/STR Xt, [Xd, #imm2]
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                instr1.uses_immediate &&
                (is_load || is_store) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.base_reg)) {
                return true;
            }

            return false;
        },
        [](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
            const auto& instr1 = instrs[pos];
            const auto& instr2 = instrs[pos + 1];

            // Pattern 1: ADD Xd, Xn, #imm; ADD Xd, Xd, Xm
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                instr1.uses_immediate &&
                instr2.opcode == InstructionDecoder::OpType::ADD &&
                !instr2.uses_immediate) {
                if (InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg) &&
                    InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1)) {
                    // ADD Xd, Xn, #imm; ADD Xd, Xd, Xm -> ADD Xd, Xn, Xm (+ ADD Xd, Xd, #imm if imm != 0)
                    Instruction opt_instr = Encoder::create_add_reg(instr2.dest_reg, instr1.src_reg1, instr2.src_reg2);
                    if (instr1.immediate != 0) {
                        Instruction add_imm_instr = Encoder::create_add_imm(instr2.dest_reg, instr2.dest_reg, instr1.immediate);
                        return { opt_instr, add_imm_instr };
                    } else {
                        return { opt_instr };
                    }
                }
            }

            // Pattern 2: ADD Xd, Xn, #imm; ADD Xd, Xd, Xm, LSL #imm2
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                instr1.uses_immediate &&
                instr2.opcode == InstructionDecoder::OpType::ADD &&
                !instr2.uses_immediate && instr2.shift_type == InstructionDecoder::ShiftType::LSL) {
                if (InstructionComparator::areSameRegister(instr1.dest_reg, instr2.dest_reg) &&
                    InstructionComparator::areSameRegister(instr1.dest_reg, instr2.src_reg1)) {
                    Instruction opt_instr = EncoderExtended::create_add_shifted_reg(
                        instr2.dest_reg, instr1.src_reg1, instr2.src_reg2, "LSL", instr2.shift_amount);
                    if (instr1.immediate != 0) {
                        Instruction add_imm_instr = Encoder::create_add_imm(instr2.dest_reg, instr2.dest_reg, instr1.immediate);
                        return { opt_instr, add_imm_instr };
                    } else {
                        return { opt_instr };
                    }
                }
            }

            // Pattern 3: ADD Xd, Xn, Xm; LDR/STR Xt, [Xd, #0] -> LDR/STR Xt, [Xn, Xm]
            bool is_load = (instr2.opcode == InstructionDecoder::OpType::LDR);
            bool is_store = (instr2.opcode == InstructionDecoder::OpType::STR);
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                !instr1.uses_immediate &&
                (is_load || is_store) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.base_reg) &&
                instr2.immediate == 0) {
                if (is_load) {
                    Instruction opt_instr = Encoder::create_ldr_scaled_reg_64bit(instr2.dest_reg, instr1.src_reg1, instr1.src_reg2, 0);
                    return { opt_instr };
                } else {
                    Instruction opt_instr = Encoder::create_str_scaled_reg_64bit(instr2.dest_reg, instr1.src_reg1, instr1.src_reg2, 0);
                    return { opt_instr };
                }
            }

            // Pattern 4: ADD Xd, Xn, #imm; LDR/STR Xt, [Xd, #imm2] -> LDR/STR Xt, [Xn, #(imm1+imm2)]
            if (instr1.opcode == InstructionDecoder::OpType::ADD &&
                instr1.uses_immediate &&
                (is_load || is_store) &&
                InstructionComparator::areSameRegister(instr1.dest_reg, instr2.base_reg)) {
                int64_t combined_offset = instr1.immediate + instr2.immediate;
                // Use a conservative offset range for ARM64
                if (combined_offset >= -256 && combined_offset <= 4095) {
                    if (is_load) {
                        Instruction opt_instr = Encoder::create_ldr_imm(instr2.dest_reg, instr1.src_reg1, combined_offset);
                        return { opt_instr };
                    } else {
                        Instruction opt_instr = Encoder::create_str_imm(instr2.dest_reg, instr1.src_reg1, combined_offset);
                        return { opt_instr };
                    }
                }
            }

            // If no optimization applies, return the original instructions
            return { instr1, instr2 };
        },
        "Address Generation Optimization"
    );
}