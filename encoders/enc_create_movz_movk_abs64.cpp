#include "Encoder.h"
#include <vector>
#include <stdexcept>

/**
 * @brief Creates a sequence of MOVZ/MOVK instructions to load a 64-bit absolute address or value.
 * @details
 * This function generates an efficient sequence of one MOVZ and up to three MOVK
 * instructions to construct a 64-bit value in a register. It omits instructions
 * for 16-bit chunks that are zero, thereby creating the shortest necessary sequence.
 * If the entire address is zero, it generates a single `MOVZ xd, #0`.
 *
 * Each generated instruction is tagged with a specific relocation type from the
 * `RelocationType` enum. This allows a linker or JIT compiler to later patch
 * the immediate values if the provided `address` is a placeholder for a symbol's
 * final location.
 *
 * For example, to load `0xDEADBEEFCAFEBABE` into `x0`:
 * 1. `MOVZ x0, #0xBABE, LSL #0`  (tag: MOVZ_MOVK_IMM_0)
 * 2. `MOVK x0, #0xCAFE, LSL #16` (tag: MOVZ_MOVK_IMM_16)
 * 3. `MOVK x0, #0xBEEF, LSL #32` (tag: MOVZ_MOVK_IMM_32)
 * 4. `MOVK x0, #0xDEAD, LSL #48` (tag: MOVZ_MOVK_IMM_48)
 *
 * @param xd The destination 64-bit register (must be an 'X' register).
 * @param address The 64-bit absolute address or value to load.
 * @param symbol The symbol name associated with the address, for relocation purposes.
 * @return A `std::vector<Instruction>` containing the sequence of MOVZ/MOVK instructions.
 */
std::vector<Instruction> Encoder::create_movz_movk_abs64(const std::string& xd, uint64_t address, const std::string& symbol) {
    std::vector<Instruction> instructions;

    // Split the 64-bit address into four 16-bit chunks.
    uint16_t chunks[4];
    chunks[0] = (address >> 0) & 0xFFFF;
    chunks[1] = (address >> 16) & 0xFFFF;
    chunks[2] = (address >> 32) & 0xFFFF;
    chunks[3] = (address >> 48) & 0xFFFF;

    // Define the corresponding relocation types for each chunk, as defined in Encoder.h.
    RelocationType relocations[4] = {
        RelocationType::MOVZ_MOVK_IMM_0,
        RelocationType::MOVZ_MOVK_IMM_16,
        RelocationType::MOVZ_MOVK_IMM_32,
        RelocationType::MOVZ_MOVK_IMM_48
    };

    // --- CORRECTED LOGIC ---
    // The first instruction MUST be MOVZ for the lowest chunk to zero the upper bits,
    // regardless of whether the chunk itself is zero.
    instructions.push_back(Encoder::create_movz_imm(xd, chunks[0], 0, relocations[0], symbol));

    // Now, iterate through the remaining chunks and generate MOVK only for non-zero ones.
    for (int i = 1; i < 4; ++i) {
        int shift = i * 16;
        instructions.push_back(Encoder::create_movk_imm(xd, chunks[i], shift, relocations[i], symbol));
    }
    // --- END OF FIX ---

    return instructions;
}