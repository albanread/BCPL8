

#include "InstructionDecoder.h"
#include <stdexcept> // For std::runtime_error

// The implementation of the new functions goes here.
namespace InstructionDecoder {

// Helper function to convert a register index to its string name (e.g., 0 -> "x0")
std::string getRegisterName(int reg_num) {
    if (reg_num >= 0 && reg_num <= 31) {
        return "x" + std::to_string(reg_num);
    } else if (reg_num >= 32 && reg_num <= 63) {
        return "d" + std::to_string(reg_num - 32);
    }
    // Handle special case registers if any, or throw an error for invalid numbers
    throw std::runtime_error("Invalid register number: " + std::to_string(reg_num));
}

// Definition for decoding LDR instructions
Instruction decodeLDR(uint32_t machine_code) {
    Instruction instr;
    // Extract the 5-bit destination register identifier (bits 0-4)
    int rt = machine_code & 0x1F;

    // Check the 'V' bit (bit 26) to determine the register type
    bool is_fp_simd_load = (machine_code >> 26) & 1;

    if (is_fp_simd_load) {
        // Floating-point LDR
        instr.opcode = OpType::LDR;
        instr.dest_reg = rt + 32; // Map to range 32-63 for FP registers
    } else {
        // Integer LDR
        instr.opcode = OpType::LDR;
        instr.dest_reg = rt; // General-purpose registers (0-31)
    }

    return instr;
}

// Definition for getting the destination register as a string
std::string getDestRegAsString(const Instruction& instr) {
    int reg_num = getDestReg(instr);
    return getRegisterName(reg_num);
}

// Definition for getting the first source register as a string
std::string getSrcReg1AsString(const Instruction& instr) {
    int reg_num = getSrcReg1(instr);
    return getRegisterName(reg_num);
}

// Definition for getting the second source register as a string
std::string getSrcReg2AsString(const Instruction& instr) {
    int reg_num = getSrcReg2(instr);
    return getRegisterName(reg_num);
}

} // namespace InstructionDecoder