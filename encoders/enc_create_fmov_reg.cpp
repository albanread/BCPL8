#include "Encoder.h"
#include "BitPatcher.h"
#include <stdexcept>
#include <string>

// ARM64 FMOV (register to register, Dn to Dm)
// Encoding: FMOV Dd, Dn
// 0x1E604000 | (Dn << 5) | Dd
// D registers are numbered D0-D31

Instruction Encoder::create_fmov_reg(const std::string& dd, const std::string& ds) {
    // Parse register numbers from "D0", "D1", etc.
    auto parse_d_reg = [](const std::string& reg) -> int {
        if (reg.size() < 2 || reg[0] != 'D') return -1;
        try {
            return std::stoi(reg.substr(1));
        } catch (...) {
            return -1;
        }
    };

    int d_dst = parse_d_reg(dd);
    int d_src = parse_d_reg(ds);

    if (d_dst < 0 || d_dst > 31 || d_src < 0 || d_src > 31) {
        throw std::runtime_error("Invalid D register for FMOV: " + dd + ", " + ds);
    }

    // FMOV Dd, Dn encoding
    BitPatcher patcher(0x1E604000);
    patcher.patch(d_src, 5, 5); // bits 5-9: source register
    patcher.patch(d_dst, 0, 5); // bits 0-4: destination register

    Instruction instr(patcher.get_value(), "FMOV " + dd + ", " + ds);
    instr.opcode = InstructionDecoder::OpType::FMOV;
    instr.dest_reg = Encoder::get_reg_encoding(dd);
    instr.src_reg1 = Encoder::get_reg_encoding(ds);
    return instr;
}