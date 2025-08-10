#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include "BitPatcher.h"
#include "Encoder.h"
/**
 * @brief (Internal Helper) Encodes a 64-bit immediate into the ARM64 bitmask immediate format.
 * @details
 * ARM64 logical instructions (AND, ORR, EOR) do not take arbitrary immediate values.
 * They use a special format that can represent a 32 or 64-bit value composed of
 * a repeating bit pattern that has been rotated. This function finds the correct
 * N, immr, and imms fields for a given immediate.
 *
 * @param immediate The immediate value to encode.
 * @param is_64bit True if the operation is 64-bit, false for 32-bit.
 * @param n_val Output for the 'N' field.
 * @param immr_val Output for the 'immr' field (rotation).
 * @param imms_val Output for the 'imms' field (element size/pattern).
 * @return True if the immediate can be encoded, false otherwise.
 */
bool Encoder::encode_bitmask_immediate(uint64_t immediate, bool is_64bit, uint32_t& n_val, uint32_t& immr_val, uint32_t& imms_val) {
    if (!is_64bit) {
        // For 32-bit operations, ensure the upper 32 bits are either all 0s or all 1s
        // matching the 31st bit, effectively zero- or sign-extending the value.
        if ((immediate & 0xFFFFFFFF00000000) != 0 && (immediate & 0xFFFFFFFF00000000) != 0xFFFFFFFF00000000) {
             // If the immediate is not a simple 32-bit value, check if it's a replicated 32-bit pattern
             if ((immediate & 0xFFFFFFFF) != (immediate >> 32)) {
                return false;
             }
        }
        immediate &= 0xFFFFFFFF; // Work with the lower 32 bits
    }

    // A value of 0 or all 1s is always valid.
    if (immediate == 0 || immediate == ~0ULL) {
        imms_val = is_64bit ? 0b111111 : 0b011111;
        immr_val = 0;
        n_val = is_64bit ? 1 : 0;
        return true;
    }

    // Iterate through element sizes (2, 4, 8, 16, 32 bits)
    for (int size = 2; size <= (is_64bit ? 64 : 32); size *= 2) {
        uint64_t mask = (1ULL << size) - 1;
        uint64_t pattern = immediate & mask;

        // Check if the pattern is just a single bit or all ones (invalid for this logic)
        if (pattern == 0 || pattern == mask) continue;

        // Replicate the pattern across 64 bits
        uint64_t replicated = 0;
        for (int i = 0; i < 64; i += size) {
            replicated |= (pattern << i);
        }

        // Check if the immediate can be formed by rotating the replicated pattern
        for (int rotation = 0; rotation < size; ++rotation) {
            uint64_t rotated = (replicated >> rotation) | (replicated << (64 - rotation));
            if (!is_64bit) {
                rotated &= 0xFFFFFFFF;
            }

            if (rotated == immediate) {
                // Found a valid encoding
                n_val = is_64bit ? 1 : 0;
                imms_val = (~(size - 1) & 0b111111) | (__builtin_popcountll(pattern) - 1);
                immr_val = rotation;
                return true;
            }
        }
    }

    return false; // Immediate cannot be encoded
}


