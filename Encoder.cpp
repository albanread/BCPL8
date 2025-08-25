#include "BitPatcher.h"
#include "Encoder.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string>

/**
 * @brief Encodes the ARM64 'BIC' (Bit Clear) instruction.
 * @details
 * This function generates the 32-bit machine code for a BIC instruction,
 * which performs a bitwise AND of the first source register with the
 * bitwise NOT of the second source register. The result is stored in the
 * destination register. The operation is: `Xd = Xn & ~Xm`.
 *
 * The BIC instruction is an alias for `AND` with an inverted second operand.
 * The encoding follows the "Data-processing (register)" format for AND
 * (shifted register) with the 'N' bit (bit 21) set to 1 to invert Rm.
 *
 * The encoding layout is:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc, S, Family**: Fixed bits identifying the instruction.
 * - **N (bit 21)**: `1` to invert the second source operand.
 * - **Rm (bits 20-16)**: The second source register `xm`.
 * - **Rn (bits 9-5)**: The first source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register (e.g., "x0", "w1").
 * @param xn The first source register (e.g., "x1", "sp").
 * @param xm The second source register to be inverted (e.g., "x2", "wzr").
 * @return An `Instruction` object containing the encoding and assembly text.
 * @throw std::invalid_argument if register names are invalid or if sizes are mixed.
 */
Instruction Encoder::create_bic_reg(const std::string& xd, const std::string& xn, const std::string& xm) {
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
                throw std::invalid_argument("Invalid register prefix in '" + reg_str + "'. Must be 'w' or 'x'.");
            }

            try {
                reg_num = std::stoul(reg_str.substr(1));
                if (reg_num > 30) {
                     throw std::out_of_range("Register number out of range for '" + reg_str + "'. Use 'wsp'/'sp' or 'wzr'/'xzr' for register 31.");
                }
            } catch (const std::logic_error&) {
                throw std::invalid_argument("Invalid register format: '" + reg_str + "'.");
            }
        }
        return {reg_num, is_64bit};
    };

    // (A) Perform self-checking by parsing and validating all register arguments first.
    auto [rd_num, rd_is_64] = parse_register(xd);
    auto [rn_num, rn_is_64] = parse_register(xn);
    auto [rm_num, rm_is_64] = parse_register(xm);

    if (!(rd_is_64 == rn_is_64 && rn_is_64 == rm_is_64)) {
        throw std::invalid_argument("Mismatched register sizes. All operands for BIC (register) must be simultaneously 32-bit (W) or 64-bit (X).");
    }

    // (B) Use the BitPatcher to construct the instruction word.
    // Base opcode for 32-bit BIC (register) is 0x0A200000.
    // This is the AND (shifted register) opcode with the 'N' bit (21) already set.
    BitPatcher patcher(0x0A200000);

    if (rd_is_64) {
        patcher.patch(1, 31, 1); // Set the sf bit for 64-bit operation.
    }

    patcher.patch(rd_num, 0, 5);  // Rd
    patcher.patch(rn_num, 5, 5);  // Rn
    patcher.patch(rm_num, 16, 5); // Rm

    // (C) Format the assembly string for the Instruction object.
    std::string assembly_text = "BIC " + xd + ", " + xn + ", " + xm;

    // (D) Return the completed Instruction object. No relocation is needed.
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::BIC;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.src_reg2 = Encoder::get_reg_encoding(xm);
    return instr;
}

/**
 * @brief Encodes the ARM64 'BFI' (Bitfield Insert) instruction.
 * @details
 * This function generates the 32-bit machine code to insert a bitfield from a
 * source register into a destination register, leaving other bits unchanged.
 * BFI is an alias for the BFM (Bitfield Move) instruction.
 * The operation is `BFI <Xd|Wd>, <Xn|Wn>, #lsb, #width`.
 *
 * The encoding follows the "Bitfield" format for BFM:
 * - **sf (bit 31)**: 1 for 64-bit, 0 for 32-bit.
 * - **opc (bits 30-29)**: `01` for BFM (BFI alias).
 * - **Family (bits 28-23)**: `0b100110`.
 * - **N (bit 22)**: Must match `sf`.
 * - **immr (bits 21-16)**: The right-rotate amount, calculated as `(datasize - lsb) % datasize`.
 * - **imms (bits 15-10)**: The most significant bit of the source field, which is `width - 1`.
 * - **Rn (bits 9-5)**: The source register `xn`.
 * - **Rd (bits 4-0)**: The destination register `xd`.
 *
 * @param xd The destination register to be modified (e.g., "x0", "w1").
 * @param xn The source register containing the bits to insert.
 * @param lsb The least significant bit (start position) in the destination (0-63).
 * @param width The width of the bitfield to insert (1-64).
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or bitfield parameters.
 */
Instruction Encoder::opt_create_bfi(const std::string& xd, const std::string& xn, int lsb, int width) {
    // 1. Validate register names and determine size
    uint32_t rd_num = get_reg_encoding(xd);
    uint32_t rn_num = get_reg_encoding(xn);
    bool is_64bit = (xd[0] == 'x' || xd[0] == 'X');

    if (is_64bit != (xn[0] == 'x' || xn[0] == 'X')) {
        throw std::invalid_argument("Mismatched register sizes for BFI.");
    }

    // 2. Validate bitfield parameters
    int datasize = is_64bit ? 64 : 32;
    if (lsb < 0 || lsb >= datasize) {
        throw std::invalid_argument("BFI lsb is out of range for the register size.");
    }
    if (width < 1 || width > datasize) {
        throw std::invalid_argument("BFI width is out of range for the register size.");
    }
    if ((lsb + width) > datasize) {
        throw std::invalid_argument("BFI bitfield (lsb + width) exceeds register size.");
    }

    // 3. Calculate encoding fields for the BFM alias
    uint32_t n_val = is_64bit ? 1 : 0;
    uint32_t immr_val = static_cast<uint32_t>((datasize - lsb) % datasize);
    uint32_t imms_val = static_cast<uint32_t>(width - 1);

    // 4. Use BitPatcher to construct the instruction word.
    // Base opcode for BFM (BFI alias) is 0x33000000.
    BitPatcher patcher(0x33000000);

    if (is_64bit) {
        patcher.patch(1, 31, 1); // sf bit
    }

    patcher.patch(n_val, 22, 1);       // N bit
    patcher.patch(immr_val, 16, 6);    // immr (rotate)
    patcher.patch(imms_val, 10, 6);    // imms (msb)
    patcher.patch(rn_num, 5, 5);       // Rn
    patcher.patch(rd_num, 0, 5);       // Rd

    // 5. Format the assembly string and return the Instruction object.
    std::string assembly_text = "BFI " + xd + ", " + xn + ", #" + std::to_string(lsb) + ", #" + std::to_string(width);
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::BFI;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.src_reg1 = Encoder::get_reg_encoding(xn);
    instr.immediate = lsb; // Store lsb in immediate field
    instr.uses_immediate = true;
    return instr;
}

/**
 * @brief Creates an ADR instruction. Loads the address of a label into a register.
 * @param xd The destination register.
 * @param label_name The target label.
 * @return A complete Instruction object with relocation info.
 *
 * Note: This is a stub implementation for use by the peephole optimizer's ADR fusion pattern.
 * It emits a pseudo-instruction with relocation info; actual encoding/linking is handled later.
 */
Instruction Encoder::create_adr(const std::string &xd, const std::string &label_name) {
    // For now, emit a dummy encoding and mark with relocation.
    // Real encoding should be handled by the linker/relocator.
    std::string assembly_text = "ADR " + xd + ", " + label_name;
    Instruction instr(0x10000000, assembly_text, RelocationType::PAGE_21_BIT_PC_RELATIVE, label_name, false);
    instr.opcode = InstructionDecoder::OpType::ADR;
    instr.dest_reg = Encoder::get_reg_encoding(xd);
    instr.target_label = label_name;
    return instr;
}
