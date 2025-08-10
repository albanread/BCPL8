#include "BitPatcher.h"
#include "Encoder.h"
#include <string>

/**
 * @brief Creates a 'RET' (Return) instruction.
 * @details
 * This function generates the machine code for a RET instruction, which performs
 * an indirect branch to the address held in the link register (X30). It is the
 * standard way to return from a function.
 *
 * While `RET` is functionally similar to `BR X30`, it uses a distinct opcode
 * that provides a hint to the processor's branch prediction hardware that a
 * function return is occurring.
 *
 * The encoding follows the "Unconditional branch (register)" format for RET:
 * - **Family (bits 31-10)**: Fixed value of `0b1101011001011111000000`.
 * - **Rn (bits 9-5)**: `30` (the Link Register, X30).
 * - **op (bits 4-0)**: Fixed `0b00000`.
 *
 * @return An `Instruction` object for `RET`.
 */
Instruction Encoder::create_return() {
    // The base opcode for the RET instruction family is 0xD65F0000.
    BitPatcher patcher(0xD65F0000);

    // Patch the source register (Rn) to the Link Register (X30).
    patcher.patch(30, 5, 5);

    // The canonical assembly text for this instruction.
    std::string assembly_text = "RET";

    // Return the completed instruction, tagging as a Jump for analysis tools.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::RET;
    instr.src_reg1 = 30; // X30 (Link Register)
    return instr;
}
