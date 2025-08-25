#include "Encoder.h"
#include <vector>
#include <stdexcept>

/**
 * @brief Creates an EFFICIENT sequence of MOVZ/MOVK instructions to load a 64-bit value.
 * @details
 * This function generates the shortest possible sequence of one MOVZ and up to three MOVK
 * instructions. It intelligently skips 16-bit chunks that are zero.
 *
 * @param xd The destination 64-bit register (must be an 'X' register).
 * @param address The 64-bit absolute address or value to load.
 * @param symbol The symbol name for relocation purposes.
 * @return A `std::vector<Instruction>` containing the minimal instruction sequence.
 */
std::vector<Instruction> Encoder::create_movz_movk_abs64(const std::string& xd, uint64_t address, const std::string& symbol) {
    std::vector<Instruction> instructions;

    // Handle the edge case where the address is zero.
    if (address == 0) {
        instructions.push_back(Encoder::create_movz_imm(xd, 0, 0, RelocationType::MOVZ_MOVK_IMM_0, symbol));
        return instructions;
    }

    // Split the 64-bit address into four 16-bit chunks.
    uint16_t chunks[4];
    chunks[0] = (address >> 0)  & 0xFFFF;
    chunks[1] = (address >> 16) & 0xFFFF;
    chunks[2] = (address >> 32) & 0xFFFF;
    chunks[3] = (address >> 48) & 0xFFFF;

    // Define the corresponding relocation types for each chunk.
    RelocationType relocations[4] = {
        RelocationType::MOVZ_MOVK_IMM_0,
        RelocationType::MOVZ_MOVK_IMM_16,
        RelocationType::MOVZ_MOVK_IMM_32,
        RelocationType::MOVZ_MOVK_IMM_48
    };

    bool is_first_instruction = true;

    // Iterate through the chunks and generate instructions only for non-zero parts.
    for (int i = 0; i < 4; ++i) {
        if (chunks[i] != 0) {
            int shift = i * 16;
            if (is_first_instruction) {
                // The first non-zero chunk uses MOVZ to zero out the rest of the register.
                instructions.push_back(Encoder::create_movz_imm(xd, chunks[i], shift, relocations[i], symbol));
                is_first_instruction = false;
            } else {
                // Subsequent non-zero chunks use MOVK to keep the existing bits.
                instructions.push_back(Encoder::create_movk_imm(xd, chunks[i], shift, relocations[i], symbol));
            }
        }
    }

    return instructions;
}
