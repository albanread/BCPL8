#include "../Encoder.h"
#include "../InstructionDecoder.h"
#include <cstdint>
#include <limits>

// Helper for ADD/SUB: 12-bit unsigned, optionally shifted by 12
static bool canEncodeAddSubImmediate(int64_t imm) {
    // 12-bit unsigned, no shift
    if (imm >= 0 && imm <= 0xFFF) return true;
    // 12-bit unsigned, shifted left by 12 (imm12 << 12)
    if ((imm & 0xFFF) == 0 && (imm >> 12) >= 0 && (imm >> 12) <= 0xFFF) return true;
    return false;
}

// Helper for logical immediates (AND/ORR/EOR): ARM64 "bitmask immediate" encoding
// This is a complex encoding; here we use a conservative approximation for common cases.
// For a full implementation, see ARM ARM C3.2.5, but here are some common encodable values:
// - All-ones (e.g., 0xFF, 0xFFFF, 0xFFFFFFFF, etc.)
// - Single contiguous runs of 1s (e.g., 0x3C, 0xF0, 0xFF00, etc.)
// - Powers of two minus one (e.g., 0x1, 0x3, 0x7, 0xF, 0x1F, ...)
// - Inverse of above (e.g., 0xFFFFFFFE, 0xFFFFFFFC, ...)
// This function is intentionally conservative: it may reject some encodable values, but will not accept invalid ones.
static bool isBitmaskImmediate(uint64_t imm, int width) {
    // Trivial cases
    if (imm == 0 || imm == std::numeric_limits<uint64_t>::max()) return false;
    if (width == 32) imm &= 0xFFFFFFFF;

    // Check for single run of 1s (e.g., 0x000FF000)
    uint64_t x = imm | (imm >> 1);
    x |= (x >> 2);
    x |= (x >> 4);
    x |= (x >> 8);
    x |= (x >> 16);
    x |= (x >> 32);
    // If x is all 1s, then imm was a single run of 1s (possibly rotated)
    if (x == (width == 32 ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF)) {
        // Now check if imm is a rotated run of 1s
        for (int rot = 0; rot < width; ++rot) {
            uint64_t rotated = ((imm << rot) | (imm >> (width - rot))) & ((1ULL << width) - 1);
            if ((rotated & (rotated + 1)) == 0) return true; // All 1s are contiguous
        }
    }

    // Common bitmask patterns
    // All-ones in a byte/halfword/word
    for (int size = 1; size <= width; size <<= 1) {
        uint64_t mask = (1ULL << size) - 1;
        for (int shift = 0; shift <= width - size; ++shift) {
            if ((imm & (mask << shift)) == (mask << shift) && (imm | (mask << shift)) == ((1ULL << width) - 1)) {
                return true;
            }
        }
    }

    // Powers of two minus one
    if ((imm & (imm + 1)) == 0) return true;
    // Inverse: powers of two minus one, inverted
    uint64_t inv = ~imm & ((1ULL << width) - 1);
    if ((inv & (inv + 1)) == 0) return true;

    return false;
}

bool Encoder::canEncodeAsImmediate(InstructionDecoder::OpType opcode, int64_t immediate) {
    using OpType = InstructionDecoder::OpType;
    switch (opcode) {
        case OpType::ADD:
        case OpType::SUB:
            // ADD/SUB (immediate): 12-bit unsigned, optionally shifted by 12
            return canEncodeAddSubImmediate(immediate);

        case OpType::AND:
        case OpType::ORR:
        case OpType::EOR:
            // Logical immediates: ARM64 bitmask immediate encoding
            // Try both 32-bit and 64-bit encodings
            return isBitmaskImmediate(static_cast<uint64_t>(immediate), 32) ||
                   isBitmaskImmediate(static_cast<uint64_t>(immediate), 64);

        // You can add more opcodes and their rules here as needed.
        default:
            // For unknown opcodes, be conservative and return false.
            return false;
    }
}