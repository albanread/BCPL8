#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include "../InstructionComparator.h"
#include "../EncoderExtended.h"
#include <iostream>
#include <algorithm>
#include <regex>
#include <cmath>

/**
 * @brief Creates a pattern to fuse MOV with ALU operations.
 * 
 * This pattern recognizes when a MOV loads an immediate value into a register
 * and that register is used as an operand in a subsequent ALU operation.
 * It combines these two instructions into a single ALU operation with an immediate,
 * reducing code size and improving execution speed.
 * 
 * Pattern detected:
 * MOVZ Xn, #imm
 * <ALU_OP> Xd, Xs, Xn -> <ALU_OP> Xd, Xs, #imm
 * 
 * Where <ALU_OP> can be ADD, SUB, AND, ORR, EOR, etc.
 * 
 * @return A unique pointer to an InstructionPattern.
 */
std::unique_ptr<InstructionPattern> PeepholeOptimizer::createFuseMOVAndALUPattern() {
    return std::make_unique<InstructionPattern>(
        2, // Pattern size of 2 instructions
        [](const std::vector<Instruction>& instrs, size_t pos) -> bool {
            if (pos + 1 >= instrs.size()) return false;

            const auto& mov_instr = instrs[pos];
            const auto& alu_instr = instrs[pos + 1];

            // ✅ STEP 1: Check opcodes and operands directly from the struct
            if (mov_instr.opcode != InstructionDecoder::OpType::MOV || !mov_instr.uses_immediate) {
                return false;
            }
            // Supported ALU operations
            if (alu_instr.opcode != InstructionDecoder::OpType::ADD &&
                alu_instr.opcode != InstructionDecoder::OpType::SUB &&
                alu_instr.opcode != InstructionDecoder::OpType::AND &&
                alu_instr.opcode != InstructionDecoder::OpType::ORR &&
                alu_instr.opcode != InstructionDecoder::OpType::EOR) {
                return false;
            }

            // Delegate the complex immediate validation to the Encoder.
            if (!Encoder::canEncodeAsImmediate(alu_instr.opcode, mov_instr.immediate)) {
                return false;
            }

            // ✅ STEP 2: Use the intelligent comparator on the reliable register numbers
            bool reg_match = InstructionComparator::areSameRegister(mov_instr.dest_reg, alu_instr.src_reg1) ||
                             InstructionComparator::areSameRegister(mov_instr.dest_reg, alu_instr.src_reg2);
            if (!reg_match) {
                return false;
            }

            // ✅ STEP 3: Perform the liveness check using the same efficient, reliable data
            for (size_t i = pos + 2; i < instrs.size(); ++i) {
                const auto& future_instr = instrs[i];
                if (InstructionComparator::areSameRegister(future_instr.dest_reg, mov_instr.dest_reg)) {
                    break; // Register is redefined, so the old value is no longer live.
                }
                if (InstructionComparator::areSameRegister(future_instr.src_reg1, mov_instr.dest_reg) ||
                    InstructionComparator::areSameRegister(future_instr.src_reg2, mov_instr.dest_reg)) {
                    return false; // The value is used later; cannot optimize away the MOV.
                }
            }

            return true; // The pattern is a valid and safe match!
        },
// [IMPROVED REPLACEMENT LOGIC]
// This lambda function is executed when the corresponding pattern is matched.
// It replaces the matched instructions with an optimized version.
[](const std::vector<Instruction>& instrs, size_t pos) -> std::vector<Instruction> {
    const auto& mov_instr = instrs[pos];
    const auto& alu_instr = instrs[pos + 1];

    // Extract the immediate value from the MOVZ instruction.
    int64_t imm_val = InstructionDecoder::getImmediate(mov_instr);

    // Get the destination and source registers from the ALU instruction.
    std::string alu_dest_reg = InstructionDecoder::getDestRegAsString(alu_instr);
    int alu_src_reg1_num = alu_instr.src_reg1;
    int alu_src_reg2_num = alu_instr.src_reg2;
    int mov_dest_reg_num = mov_instr.dest_reg;

    // Determine which of the ALU's source operands matches the MOV's destination.
    // This tells us which register to replace with the immediate value.
    bool mov_is_src1 = InstructionComparator::areSameRegister(mov_dest_reg_num, alu_src_reg1_num);

    // The other source register from the ALU op will be the source for our new immediate-based instruction.
    std::string new_src_reg;
    if (mov_is_src1) {
        // If the MOV'd register was the first operand, the second operand is our new source.
        new_src_reg = InstructionDecoder::getSrcReg2AsString(alu_instr);
    } else {
        // Otherwise, the first operand is our new source.
        new_src_reg = InstructionDecoder::getSrcReg1AsString(alu_instr);
    }

    // Use a switch statement to handle the different ALU operations cleanly.
    auto alu_opcode = InstructionDecoder::getOpcode(alu_instr);
    Instruction optimized_instr;

    switch (alu_opcode) {
        case InstructionDecoder::OpType::ADD:
            optimized_instr = Encoder::create_add_imm(alu_dest_reg, new_src_reg, imm_val);
            break;

        case InstructionDecoder::OpType::SUB:
             // Special case: `SUB Xd, Xn, Xm` where Xn is from the MOV.
             // This becomes `SUB Xd, #imm, Xm`, which is not a valid ARM instruction.
             // We must fall back to the original instructions in this case.
            if (mov_is_src1) {
                return { mov_instr, alu_instr };
            }
            optimized_instr = Encoder::create_sub_imm(alu_dest_reg, new_src_reg, imm_val);
            break;

        case InstructionDecoder::OpType::AND:
            optimized_instr = Encoder::opt_create_and_imm(alu_dest_reg, new_src_reg, imm_val);
            break;

        case InstructionDecoder::OpType::ORR:
            optimized_instr = Encoder::opt_create_orr_imm(alu_dest_reg, new_src_reg, imm_val);
            break;

        case InstructionDecoder::OpType::EOR:
            optimized_instr = Encoder::opt_create_eor_imm(alu_dest_reg, new_src_reg, imm_val);
            break;

        case InstructionDecoder::OpType::CMP: // CMP is an alias for SUBS Rd, Rn, Rm where Rd is XZR
            // Similar to SUB, `CMP #imm, Xm` is not valid.
            if (mov_is_src1) {
                return { mov_instr, alu_instr };
            }
            optimized_instr = Encoder::create_cmp_imm(new_src_reg, imm_val);
            break;

        default:
            // SAFETY NET: If the opcode is not one we can handle,
            // return the original instructions to prevent generating an illegal instruction.
            return { mov_instr, alu_instr };
    }

    // Final check to ensure a valid instruction was created before returning.
    if (optimized_instr.encoding == 0) {
         // This should ideally not be reached due to the default case, but serves as a final safeguard.
        std::cerr << "Warning: Peephole optimizer failed to create a valid encoding for a fused instruction. Reverting." << std::endl;
        return { mov_instr, alu_instr };
    }

    return { optimized_instr };
}
        "Fuse MOV with ALU Operations"
    );
}