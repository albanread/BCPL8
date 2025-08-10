#include "../PeepholeOptimizer.h"
#include "../EncoderExtended.h"
#include "../InstructionDecoder.h"
#include <iostream>
#include <algorithm>
#include <regex>

/**
 * @brief Enhanced tracing for optimization with before/after comparison
 * 
 * @param description Description of the optimization being applied
 * @param before Original instructions before optimization
 * @param after Optimized instructions after optimization
 * @param position Position in the instruction stream where the optimization is applied
 */
void PeepholeOptimizer::traceOptimization(const std::string& description, 
                                         const std::vector<Instruction>& before,
                                         const std::vector<Instruction>& after,
                                         size_t position) const {
    if (!enable_tracing_) return;

    std::cout << "\n[OPTIMIZE] Position " << position << ": " << description << "\n";
    
    // Show before instructions
    std::cout << "  BEFORE (" << before.size() << " instructions):\n";
    for (size_t i = 0; i < before.size(); i++) {
        std::cout << "    " << position + i << ": " << before[i].assembly_text << "\n";
    }
    
    // Show after instructions
    std::cout << "  AFTER (" << after.size() << " instructions):\n";
    for (size_t i = 0; i < after.size(); i++) {
        std::cout << "    " << position + i << ": " << after[i].assembly_text << "\n";
    }
    
    // Show change statistics
    int delta = static_cast<int>(after.size()) - static_cast<int>(before.size());
    std::cout << "  RESULT: ";
    if (delta < 0) {
        std::cout << "Removed " << -delta << " instructions\n";
    } else if (delta > 0) {
        std::cout << "Added " << delta << " instructions\n";
    } else {
        std::cout << "Same number of instructions\n";
    }
    std::cout << "\n";
}

/**
 * @brief Check if an instruction is a special instruction that should not be modified.
 * Special instructions include labels, directives, and other non-modifiable instructions.
 * 
 * @param instr The instruction to check
 * @return true if the instruction is special
 * @return false if the instruction can be modified
 */
bool PeepholeOptimizer::isSpecialInstruction(const Instruction& instr) const {
    // Labels and directives are special instructions
    if (instr.is_label_definition || instr.assembly_text.find(".") == 0) {
        return true;
    }
    
    // Data values are also special
    if (instr.is_data_value) {
        return true;
    }

    // Instructions with JIT attributes should not be modified
    if (instr.jit_attribute != JITAttribute::None) {
        return true;
    }

    // Use semantic opcode field for special instructions
    InstructionDecoder::OpType opcode = InstructionDecoder::getOpcode(instr);
    static const std::vector<InstructionDecoder::OpType> special_opcodes = {
        InstructionDecoder::OpType::SVC,
        InstructionDecoder::OpType::BRK,
        InstructionDecoder::OpType::DMB,
        InstructionDecoder::OpType::ISB,
        InstructionDecoder::OpType::DSB,
        InstructionDecoder::OpType::MSR,
        InstructionDecoder::OpType::MRS,
        InstructionDecoder::OpType::RET,
        InstructionDecoder::OpType::BL,
        InstructionDecoder::OpType::NOP,
        InstructionDecoder::OpType::UDF,
        InstructionDecoder::OpType::BKPT
        // Add more as needed
    };
    for (const auto& special_op : special_opcodes) {
        if (opcode == special_op) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Check if applying an optimization would break any label references
 * 
 * @param instructions The current instruction stream
 * @param start_pos The starting position for the optimization
 * @param count The number of instructions to be replaced
 * @param replacements The replacement instructions
 * @return true if the optimization would break label references
 * @return false if the optimization is safe
 */
bool PeepholeOptimizer::wouldBreakLabelReferences(
    const std::vector<Instruction>& instructions,
    size_t start_pos, size_t count,
    const std::vector<Instruction>& replacements) const {
    
    // If we're removing more instructions than we're adding, check that none of 
    // the removed instructions are label definitions (i.e., are referenced by a label)
    if (count > replacements.size()) {
        for (size_t i = 0; i < count; i++) {
            const auto& instr = instructions[start_pos + i];
            
            // If instruction is a label definition, we can't safely remove it
            if (instr.is_label_definition) {
                return true;
            }
        }
    }
    
    return false;
}

/**
 * @brief Check if two registers are the same
 * 
 * @param instr1 First register name (e.g., "x0", "w1")
 * @param instr2 Second register name (e.g., "x0", "w1")
 * @return true if they represent the same physical register
 * @return false otherwise
 */

           (instr1 == "xzr" && instr2 == "wzr") ||
           (instr1 == "wzr" && instr2 == "xzr") ||
           (instr1 == "sp" && (instr2 == "sp" || instr2 == "x31" || instr2 == "w31")) ||
           (instr2 == "sp" && (instr1 == "sp" || instr1 == "x31" || instr1 == "w31"));
}

/**
 * @brief Extract register name from assembly text
 * 
 * @param assembly_text Assembly text to extract from
 * @return std::string Register name, or empty string if not found
 */
