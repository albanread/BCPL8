#include "RegisterManager.h"

/**
 * @brief Releases a register, making it available for future use.
 *
 * This function intelligently determines if the register is a general-purpose
 * or floating-point register and calls the appropriate internal release function.
 * This is crucial for correctly managing the distinct register pools.
 *
 * @param reg_name The name of the register to release (e.g., "X10", "D15").
 */
void RegisterManager::release_register(const std::string& reg_name) {
    // Check if the register is a floating-point/SIMD register (e.g., "D0", "S15").
    if (is_fp_register(reg_name)) {
        // If it is, use the specific function for releasing FP registers, which correctly
        // handles registers from both the FP_VARIABLE_REGS and FP_SCRATCH_REGS pools.
        release_fp_register(reg_name);
    } else {
        // Otherwise, assume it's a general-purpose register and use the
        // function for releasing scratch registers (e.g., "X10").
        release_scratch_reg(reg_name);
    }
}