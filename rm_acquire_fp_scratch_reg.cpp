#include "RegisterManager.h"
#include <stdexcept>

std::string RegisterManager::acquire_fp_scratch_reg() {
    for (const auto& reg : FP_SCRATCH_REGS) {
        if (registers[reg].status == FREE) {
            registers[reg].status = IN_USE_SCRATCH;
            registers[reg].bound_to = "";
            return reg;
        }
    }
    throw std::runtime_error("No available FP scratch registers.");
}