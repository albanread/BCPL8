#ifndef BITPATCHER_H
#define BITPATCHER_H

#include <cstdint> // Required for uint32_t

/**
 * @class BitPatcher
 * @brief A utility to manipulate a 32-bit data word using precise bit patching.
 *
 * This class provides a clean interface for setting, getting, and patching
 * specific bitfields within a 32-bit unsigned integer. The core patching
 * logic is intended to be implemented with inline assembly for precision,
 * making it suitable for low-level tasks like compiler instruction encoding.
 */
class BitPatcher {
public:
    /**
     * @brief Constructs a BitPatcher object.
     * @param initial_value The initial 32-bit value to be managed by the object.
     * Defaults to 0.
     */
    explicit BitPatcher(uint32_t initial_value = 0);

    /**
     * @brief Patches a value into a specific bitfield of the internal data word.
     *
     * This method takes a value and inserts its lower `num_bits` bits into the
     * data word starting at `start_bit`.
     *
     * @param value_to_patch The value to insert. Only the least significant
     * `num_bits` of this value will be used.
     * @param start_bit The starting bit position (0-31) for the patch. Bit 0 is the LSB.
     * @param num_bits The number of bits in the field to patch (1-32).
     */
    void patch(uint32_t value_to_patch, int start_bit, int num_bits);

    /**
     * @brief Retrieves the current 32-bit data word.
     * @return The current value of the internal 32-bit data word.
     */
    uint32_t get_value() const;

    /**
     * @brief Overwrites the entire 32-bit data word with a new value.
     * @param new_value The new 32-bit value to set.
     */
    void set_value(uint32_t new_value);

private:
    /// @brief The 32-bit data word that is being manipulated.
    uint32_t data;
};

#endif // BITPATCHER_H
