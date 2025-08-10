#ifndef ENCODER_EXTENDED_H
#define ENCODER_EXTENDED_H

#include "Encoder.h"
#include <cmath>
#include <unordered_map>

/**
 * @brief Extended Encoder class with additional instruction generation capabilities.
 */
class EncoderExtended {
public:
    // New method to contain the logic
    static bool isSpecialRegister(const std::string& reg) {
        if (reg == "sp" || reg == "xzr" || reg == "wzr") {
            return true;
        }
        char regType = reg[0];
        if (regType == 'x' || regType == 'w' || regType == 'v') {
            return true;
        }
        return false;
    }

    // Helper function to encode register numbers
    static int encode_register(const std::string& reg) {
        // Handle special registers
        if (reg == "sp" || reg == "xzr" || reg == "wzr") {
            return 31;
        }
        if (reg == "lr") {
            return 30;
        }
        
        // Extract number from regular registers (x0, w1, etc.)
        char regType = reg[0];
        if (regType == 'x' || regType == 'w' || regType == 'v') {
            try {
                return std::stoi(reg.substr(1));
            } catch (...) {
                return 0; // Default to x0/w0/v0 if parsing fails
            }
        }
        
        return 0;
    }

    /**
     * @brief Checks if a number is a power of two.
     * @param n The number to check.
     * @return True if the number is a power of two, false otherwise.
     */
    static bool isPowerOfTwo(int64_t n) {
        return n > 0 && (n & (n - 1)) == 0;
    }

    /**
     * @brief Calculates the log base 2 of a power of two.
     * @param n The power of two number.
     * @return The log base 2 of the number, or -1 if not a power of two.
     */
    static int log2OfPowerOfTwo(int64_t n) {
        if (!isPowerOfTwo(n)) return -1;
        
        // Calculate log2 using bit manipulation
        int log = 0;
        while (n > 1) {
            n >>= 1;
            log++;
        }
        
        return log;
    }
};

#endif // ENCODER_EXTENDED_H