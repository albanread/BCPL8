#include "../BitPatcher.h"
#include "../Encoder.h"
#include "../InstructionDecoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

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

// Forward declaration of the helper function from enc_create_branch_conditional.cpp
uint32_t get_condition_code(const std::string& cond);

// Helper to invert a condition code by flipping its least significant bit.
// For example, EQ (0b0000) becomes NE (0b0001).
uint32_t invert_condition_code(uint32_t cond) {
    return cond ^ 1;
}

/**
 * @brief Encodes the ARM64 'CSET' (Conditional Set) instruction.
 * @details
 * This function generates the machine code to set the destination register to 1
 * if a condition is TRUE, and 0 otherwise.
 *
 * `CSET` is an alias for the `CSINC` (Conditional Set Increment) instruction.
 * [cite_start]The operation `CSET Rd, cond` is encoded as `CSINC Rd, ZR, ZR, invert(cond)`[cite: 3].
 * The `CSINC` encoding follows the "Conditional select" format.
 *
 * @param Rd The destination register (e.g., "x0", "w1").
 * @param cond The condition mnemonic as a string (e.g., "EQ", "LT", "GE").
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument for invalid registers or unrecognized conditions.
 */
Instruction Encoder::create_cset(const std::string& Rd, const std::string& cond) {
    // (A) Get the numeric condition code and invert it for the CSINC alias.
    uint32_t cond_code = get_condition_code(cond);
    uint32_t inverted_cond_code = invert_condition_code(cond_code);

    // Helper lambda to parse the destination register.
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) throw std::invalid_argument("Register string cannot be empty.");
        std::string lower_reg = reg_str;
        std::transform(lower_reg.begin(), lower_reg.end(), lower_reg.begin(), ::tolower);
        bool is_64bit;
        uint32_t reg_num;
        if (lower_reg == "wzr") {
            is_64bit = false;
            reg_num = 31;
        } else if (lower_reg == "xzr") {
            is_64bit = true;
            reg_num = 31;
        } else {
            char prefix = lower_reg[0];
            if (prefix == 'w') is_64bit = false;
            else if (prefix == 'x') is_64bit = true;
            else throw std::invalid_argument("Invalid register prefix in '" + reg_str + "'. Must be 'w' or 'x'.");
            try {
                reg_num = std::stoul(reg_str.substr(1));
                if (reg_num > 31) throw std::out_of_range("Register number out of range.");
            } catch(...) {
                throw std::invalid_argument("Invalid register format: '" + reg_str + "'.");
            }
        }
        return {reg_num, is_64bit};
    };

    auto [rd_num, rd_is_64] = parse_register(Rd);

    // (B) Use BitPatcher to encode CSINC Rd, ZR, ZR, !cond.
    // The base opcode for 32-bit CSINC is 0x1A800400.
    BitPatcher patcher(0x1A800400);

    // Patch sf for 64-bit operation.
    if (rd_is_64) {
        patcher.patch(1, 31, 1);
    }

    // Patch destination register (Rd)
    patcher.patch(rd_num, 0, 5);

    // Patch source registers (Rn and Rm) to be the zero register (31)
    patcher.patch(31, 5, 5);  // Rn = ZR
    patcher.patch(31, 16, 5); // Rm = ZR

    // Patch the INVERTED condition code
    patcher.patch(inverted_cond_code, 12, 4);

    // (C) Format the assembly string for the CSET alias.
    std::string upper_cond = cond;
    std::transform(upper_cond.begin(), upper_cond.end(), upper_cond.begin(), ::toupper);
    std::string assembly_text = "CSET " + Rd + ", " + upper_cond;

    // (D) Return the completed Instruction object.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::CSET;
    instr.dest_reg = Encoder::get_reg_encoding(Rd);

    // Set the semantic condition code field using the helper
    instr.cond = stringToConditionCode(cond);

    return instr;
}
