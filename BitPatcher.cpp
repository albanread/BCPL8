#include "BitPatcher.h"
#include <stdexcept> // Required for std::out_of_range

/**
 * @brief Constructs a BitPatcher object.
 * @param initial_value The initial 32-bit value.
 */
BitPatcher::BitPatcher(uint32_t initial_value) : data(initial_value) {}

/**
 * @brief Retrieves the current 32-bit data word.
 * @return The current value of the internal data.
 */
uint32_t BitPatcher::get_value() const {
    return this->data;
}

/**
 * @brief Overwrites the entire 32-bit data word.
 * @param new_value The new 32-bit value to set.
 */
void BitPatcher::set_value(uint32_t new_value) {
    this->data = new_value;
}

/**
 * @brief Patches a value into a specific bitfield of the internal data word.
 *
 * @param value_to_patch The value to insert.
 * @param start_bit The starting bit position (0-31).
 * @param num_bits The number of bits in the field to patch (1-32).
 */
void BitPatcher::patch(uint32_t value_to_patch, int start_bit, int num_bits) {
    // --- C++ Input Validation ---
    if (num_bits <= 0 || num_bits > 32) {
        throw std::out_of_range("BitPatcher::patch - num_bits must be between 1 and 32.");
    }
    if (start_bit < 0 || start_bit > 31) {
        throw std::out_of_range("BitPatcher::patch - start_bit must be between 0 and 31.");
    }
    if ((start_bit + num_bits) > 32) {
        throw std::out_of_range("BitPatcher::patch - start_bit + num_bits cannot exceed 32.");
    }

    // --- ARM64 Inline Assembly ---
    uint32_t temp_mask;

    __asm__ volatile (
        // 1. Create a mask of '1's for the given number of bits.
        "CMP %w[num_bits], #32 \n\t"
        "B.EQ 1f \n\t"
        "MOV w1, #1 \n\t"
        "LSL w1, w1, %w[num_bits] \n\t"
        "SUB w1, w1, #1 \n\t"
        "B 2f \n\t"
        "1: \n\t"
        "MOV w1, #0xFFFFFFFF \n\t"
        "2: \n\t"

        // 2. Shift the field mask to the correct starting bit position.
        "LSL %w[mask], w1, %w[start_bit] \n\t"

        // 3. Clear the target bitfield in the main data word.
        "MVN w2, %w[mask] \n\t"
        "AND %w[data], %w[data], w2 \n\t"

        // 4. Shift the patch value to the correct starting bit position.
        "LSL %w[val], %w[val], %w[start_bit] \n\t"

        // 5. Mask the SHIFTED value with the SHIFTED field mask.
        "AND %w[val], %w[val], %w[mask] \n\t"

        // 6. OR the now-correctly-masked and shifted value into the main data word.
        "ORR %w[data], %w[data], %w[val] \n\t"

        // --- Operand Constraints ---
        : [data] "+r" (this->data),
          [mask] "=&r" (temp_mask),
          [val] "+r" (value_to_patch)
        : [start_bit] "r" (start_bit),
          [num_bits] "r" (num_bits)
        : "w1", "w2", "cc" // Clobbered registers and condition codes.
    );
}

