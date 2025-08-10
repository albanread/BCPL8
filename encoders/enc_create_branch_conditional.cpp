#include "../BitPatcher.h"
#include "../Encoder.h"
#include "../InstructionDecoder.h"
#include <algorithm>
#include <cctype>
#include <map>
#include <stdexcept>
#include <string>



/**
 * @brief Creates a conditional branch instruction (e.g., B.EQ) to a label.
 * @details
 * This function generates the machine code for a conditional branch. The branch
 * target is a PC-relative offset encoded as a 19-bit signed immediate.
 *
 * The immediate field is set to 0, and the instruction is tagged with
 * `RelocationType::PC_RELATIVE_19_BIT_OFFSET`. The linker is responsible for
 * calculating the final offset and patching the instruction.
 *
 * The encoding follows the "Conditional branch (immediate)" format[cite: 90]:
 * - **Family (bits 31-25)**: `0b0101010`.
 * - **op (bit 24)**: `0`.
 * - **imm19 (bits 23-5)**: The 19-bit signed PC-relative immediate (offset / 4).
 * - **Fixed (bit 4)**: `0`.
 * - **cond (bits 3-0)**: The 4-bit condition code from `get_condition_code`.
 *
 * @param condition The condition code as a string (e.g., "EQ", "NE", "LT").
 * @param label_name The target label.
 * @return An `Instruction` object with relocation info.
 * @throw std::invalid_argument if the condition is not recognized.
 */
namespace {
ConditionCode stringToConditionCode(const std::string& cond) {
    std::string upper_cond = cond;
    std::transform(upper_cond.begin(), upper_cond.end(), upper_cond.begin(), ::toupper);
    if (upper_cond == "EQ") return ConditionCode::EQ;
    if (upper_cond == "NE") return ConditionCode::NE;
    if (upper_cond == "CS") return ConditionCode::CS;
    if (upper_cond == "CC") return ConditionCode::CC;
    if (upper_cond == "MI") return ConditionCode::MI;
    if (upper_cond == "PL") return ConditionCode::PL;
    if (upper_cond == "VS") return ConditionCode::VS;
    if (upper_cond == "VC") return ConditionCode::VC;
    if (upper_cond == "HI") return ConditionCode::HI;
    if (upper_cond == "LS") return ConditionCode::LS;
    if (upper_cond == "GE") return ConditionCode::GE;
    if (upper_cond == "LT") return ConditionCode::LT;
    if (upper_cond == "GT") return ConditionCode::GT;
    if (upper_cond == "LE") return ConditionCode::LE;
    if (upper_cond == "AL") return ConditionCode::AL;
    if (upper_cond == "NV") return ConditionCode::NV;
    return ConditionCode::UNKNOWN;
}
}

Instruction Encoder::create_branch_conditional(const std::string& condition, const std::string& label_name) {
    // (A) Get the 4-bit encoding for the condition string.
    uint32_t cond_code = get_condition_code(condition);

    // (B) Use BitPatcher. The base for B.cond is 0x54000000.
    // The immediate field (imm19) is left as zero for the linker to patch.
    BitPatcher patcher(0x54000000);

    // Patch the condition code into bits 0-3.
    patcher.patch(cond_code, 0, 4);

    // (C) Format the assembly string with a capitalized condition.
    std::string upper_condition = condition;
    std::transform(upper_condition.begin(), upper_condition.end(), upper_condition.begin(), ::toupper);
    std::string assembly_text = "B." + upper_condition + " " + label_name;

    // (D) Return the completed Instruction object with relocation information.
    Instruction instr(patcher.get_value(), assembly_text);

    // ✅ Set the opcode field
    instr.opcode = InstructionDecoder::OpType::B_COND;
    instr.relocation = RelocationType::PC_RELATIVE_19_BIT_OFFSET;
    instr.target_label = label_name;

    // ✅ Set the semantic condition code field using the helper
    instr.cond = stringToConditionCode(condition);

    return instr;
}
