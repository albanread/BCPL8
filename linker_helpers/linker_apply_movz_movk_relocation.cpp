#include "Linker.h"
#include "BitPatcher.h"
#include <stdexcept>
#include <cstdint>

/**
 * @brief Patches MOVZ/MOVK instructions used for loading absolute 64-bit addresses.
 *
 * @param instruction_encoding The original instruction encoding.
 * @param target_address The absolute address to be loaded.
 * @param type The relocation type (which 16 bits to patch).
 * @return The patched instruction encoding.
 */
uint32_t Linker::apply_movz_movk_relocation(
    uint32_t instruction_encoding,
    size_t target_address,
    RelocationType type
) {
    BitPatcher patcher(instruction_encoding);
    uint16_t imm16 = 0;

    switch (type) {
        case RelocationType::MOVZ_MOVK_IMM_0:
            imm16 = (target_address >> 0) & 0xFFFF;
            break;
        case RelocationType::MOVZ_MOVK_IMM_16:
            imm16 = (target_address >> 16) & 0xFFFF;
            break;
        case RelocationType::MOVZ_MOVK_IMM_32:
            imm16 = (target_address >> 32) & 0xFFFF;
            break;
        case RelocationType::MOVZ_MOVK_IMM_48:
            imm16 = (target_address >> 48) & 0xFFFF;
            break;
        default:
            throw std::runtime_error("Unsupported MOVZ/MOVK relocation type.");
    }
    patcher.patch(imm16, 5, 16);
    return patcher.get_value();
}