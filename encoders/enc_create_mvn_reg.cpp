#include "BitPatcher.h"
#include "../Encoder.h"
#include <algorithm>
#include <cctype>
#include <sstream>
#include <stdexcept>

/**
 * @brief Creates an MVN (Move with NOT) instruction, which is an alias for ORN with XZR.
 * @param xd The destination register.
 * @param xm The source register to be bitwise NOT-ed.
 * @return A complete Instruction object.
 */
Instruction Encoder::create_mvn_reg(const std::string& xd, const std::string& xm) {
    // MVN xd, xm  <=>  ORN xd, XZR, xm
    // ARM64: ORN <Xd|Wd>, XZR, <Xm|Wm>
    // We'll emit the assembly as "MVN xd, xm" for clarity.

    // Helper lambda to parse register strings like "x0", "w1", "sp", "wzr".
    auto parse_register = [](const std::string& reg_str) -> std::pair<uint32_t, bool> {
        if (reg_str.empty()) {
            throw std::invalid_argument("Register string cannot be empty.");
        }

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
        } else if (lower_reg == "wsp") {
            is_64bit = false;
            reg_num = 31;
        } else if (lower_reg == "sp") {
            is_64bit = true;
            reg_num = 31;
        } else {
            char prefix = lower_reg[0];
            if (prefix == 'w') {
                is_64bit = false;
            } else if (prefix == 'x') {
                is_64bit = true;
            } else {
                throw std::invalid_argument("Invalid register prefix: " + reg_str);
            }
            reg_num = std::stoi(lower_reg.substr(1));
        }
        return {reg_num, is_64bit};
    };

    auto [rd_num, rd_is_64] = parse_register(xd);
    auto [rm_num, rm_is_64] = parse_register(xm);

    // ORN <Xd|Wd>, XZR, <Xm|Wm>
    // Encoding for ORN (register): same as ORR (register) but with N=1 (bit 21)
    // Base opcode for 32-bit ORN (register) is 0x2A200800.
    BitPatcher patcher(0x2A200800);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    patcher.patch(rd_num, 0, 5);  // Rd
    patcher.patch(31, 5, 5);      // Rn = XZR/WZR (always 31)
    patcher.patch(rm_num, 16, 5); // Rm

    // Format the assembly string for the Instruction object.
    std::ostringstream oss;
    oss << "MVN " << xd << ", " << xm;

    Instruction instr(patcher.get_value(), oss.str());
    instr.opcode = InstructionDecoder::OpType::ORR;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding("XZR");
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    return instr;
}