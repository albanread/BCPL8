#include <algorithm>
#include <cstdint>
#include "BitPatcher.h"
#include "Encoder.h"
#include <string>
#include <stdexcept>
#include <cctype>

/**
 * @brief Encodes the ARM64 'FMADD' (Fused Multiply-Add) instruction.
 * @details
 * This function generates the machine code to multiply two source registers,
 * add a third, and store the result, without intermediate rounding.
 * The operation is `FMADD <Vd>, <Vn>, <Vm>, <Va>`, where Vd = (Vn * Vm) + Va.
 *
 * The encoding follows the "Floating-point data-processing (3 source)" format:
 * - **M (bit 31)**: `0`.
 * - **S (bit 30)**: `0`.
 * - **type (bits 23-22)**: `00` for single-precision (S), `01` for double-precision (D).
 * - **Family (bits 24, 21, 15)**: `0b11111` identifies the MADD/MSUB family.
 * - **o1 (bit 21)**: `0` for FMADD.
 * - **o0 (bit 15)**: `0` for addition.
 * - **Vm (bits 20-16)**: The second multiplicand register.
 * - **Va (bits 14-10)**: The addend register.
 * - **Vn (bits 9-5)**: The first multiplicand register.
 * - **Vd (bits 4-0)**: The destination register.
 *
 * @param vd The destination and addend register (e.g., "D0", "S1").
 * @param vn The first source register (multiplicand).
 * @param vm The second source register (multiplier).
 * @param va The third source register (addend).
 * @return An `Instruction` object.
 * @throw std::invalid_argument for invalid registers or mismatched precision.
 */
Instruction Encoder::opt_create_fmadd(const std::string& vd, const std::string& vn, const std::string& vm, const std::string& va) {
    // 1. Validate register names and determine precision
    char vd_prefix = vd.empty() ? ' ' : static_cast<char>(std::toupper(vd[0]));
    char vn_prefix = vn.empty() ? ' ' : static_cast<char>(std::toupper(vn[0]));
    char vm_prefix = vm.empty() ? ' ' : static_cast<char>(std::toupper(vm[0]));
    char va_prefix = va.empty() ? ' ' : static_cast<char>(std::toupper(va[0]));

    if (vd_prefix != vn_prefix || vn_prefix != vm_prefix || vm_prefix != va_prefix) {
        throw std::invalid_argument("Mismatched register precision for FMADD. All must be 'S' or 'D'.");
    }
    if (vd_prefix != 'S' && vd_prefix != 'D') {
        throw std::invalid_argument("Invalid register type for FMADD. Must be 'S' or 'D' registers.");
    }

    bool is_double = (vd_prefix == 'D');

    uint32_t vd_num = get_reg_encoding(vd);
    uint32_t vn_num = get_reg_encoding(vn);
    uint32_t vm_num = get_reg_encoding(vm);
    uint32_t va_num = get_reg_encoding(va);

    // 2. Use BitPatcher to construct the instruction word.
    // Base opcode for FMADD is 0x1F000000.
    BitPatcher patcher(0x1F000000);

    if (is_double) {
        patcher.patch(1, 22, 2); // type = 01 for double-precision
    }
    // type = 00 for single-precision is the default in the base opcode.

    patcher.patch(vm_num, 16, 5);    // Vm
    patcher.patch(va_num, 10, 5);    // Va
    patcher.patch(vn_num, 5, 5);     // Vn
    patcher.patch(vd_num, 0, 5);     // Vd

    // 3. Format the assembly string and return the Instruction object.
    std::string assembly_text = "FMADD " + vd + ", " + vn + ", " + vm + ", " + va;
    Instruction instr(patcher.get_value(), assembly_text);
    instr.opcode = InstructionDecoder::OpType::FMADD;
    instr.dest_reg = Encoder::get_reg_encoding(vd);
    instr.src_reg1 = Encoder::get_reg_encoding(vn);
    instr.src_reg2 = Encoder::get_reg_encoding(vm);
    instr.ra_reg = Encoder::get_reg_encoding(va);
    return instr;
}