#include "Linker.h"
#include "BitPatcher.h"
#include <stdexcept>
#include <cstdint>

/**
 * @brief Patches PC-relative instructions like B, BL, B.cond, ADRP, and ADD.
 */
uint32_t Linker::apply_pc_relative_relocation(
    uint32_t instruction_encoding,
    size_t instruction_address,
    size_t target_address,
    RelocationType type,
    bool enable_tracing
) {
    BitPatcher patcher(instruction_encoding);
    int64_t offset = static_cast<int64_t>(target_address) - static_cast<int64_t>(instruction_address);

    switch (type) {
        case RelocationType::PC_RELATIVE_26_BIT_OFFSET: { // For B, BL
            int32_t imm26 = (offset / 4) & 0x03FFFFFF;
            patcher.patch(imm26, 0, 26);
            break;
        }
        case RelocationType::PC_RELATIVE_19_BIT_OFFSET: { // For B.cond
            int32_t imm19 = (offset / 4) & 0x0007FFFF;
            patcher.patch(imm19, 5, 19);
            break;
        }
        case RelocationType::PAGE_21_BIT_PC_RELATIVE: { // For ADRP
            size_t pc_page = instruction_address & ~0xFFF;
            size_t target_page = target_address & ~0xFFF;
            int64_t page_offset = static_cast<int64_t>(target_page) - static_cast<int64_t>(pc_page);
            int32_t imm21 = (page_offset >> 12) & 0x1FFFFF;
            patcher.patch(imm21 & 0x3, 29, 2);    // immlo
            patcher.patch(imm21 >> 2, 5, 19);   // immhi
            break;
        }
        case RelocationType::ADD_12_BIT_UNSIGNED_OFFSET: { // For ADD (literal)
            uint32_t lo12 = target_address & 0xFFF;
            patcher.patch(lo12, 10, 12);
            break;
        }
        default:
            throw std::runtime_error("Unsupported PC-relative relocation type.");
    }
    return patcher.get_value();
}