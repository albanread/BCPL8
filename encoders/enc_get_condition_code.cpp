#include "Encoder.h"
#include <string>
#include <unordered_map>
#include <stdexcept>
#include <algorithm>
#include <cctype>

/**
 * @brief Maps a condition string (e.g., "EQ", "NE") to its 4-bit encoding.
 * @details
 * This helper function translates a standard ARM condition mnemonic string into
 * its corresponding 4-bit integer representation. It's not case-sensitive and supports
 * common aliases (e.g., "HS" for "CS") as found in the ARM documentation.
 *
 * @param cond The condition mnemonic as a string (e.g., "EQ", "ne", "Lt").
 * @return The 4-bit integer code for the condition.
 * @throw std::invalid_argument if the condition is not recognized.
 */
uint32_t get_condition_code(const std::string& cond) {
    // A static map is efficient as it's initialized only once.
    static const std::unordered_map<std::string, uint32_t> cond_map = {
        {"EQ", 0b0000}, // Equal [cite: 218]
        {"NE", 0b0001}, // Not equal [cite: 218]
        {"CS", 0b0010}, // Carry set [cite: 218]
        {"HS", 0b0010}, // Unsigned higher or same [cite: 218]
        {"CC", 0b0011}, // Carry clear [cite: 218]
        {"LO", 0b0011}, // Unsigned lower [cite: 218]
        {"MI", 0b0100}, // Minus, Negative [cite: 218]
        {"PL", 0b0101}, // Plus, Positive or zero [cite: 218]
        {"VS", 0b0110}, // Overflow [cite: 218]
        {"VC", 0b0111}, // No overflow [cite: 218]
        {"HI", 0b1000}, // Unsigned higher [cite: 218]
        {"LS", 0b1001}, // Unsigned lower or same [cite: 218]
        {"GE", 0b1010}, // Signed greater than or equal [cite: 218]
        {"LT", 0b1011}, // Signed less than [cite: 218]
        {"GT", 0b1100}, // Signed greater than [cite: 218]
        {"LE", 0b1101}, // Signed less than or equal [cite: 218]
        {"AL", 0b1110}  // Always [cite: 218]
    };

    // Convert input to uppercase for case-insensitive matching.
    std::string upper_cond = cond;
    std::transform(upper_cond.begin(), upper_cond.end(), upper_cond.begin(), ::toupper);

    auto it = cond_map.find(upper_cond);
    if (it == cond_map.end()) {
        throw std::invalid_argument("Invalid condition mnemonic: " + cond);
    }
    return it->second;
}
