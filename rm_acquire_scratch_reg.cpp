#include "RegisterManager.h"
#include <stdexcept>

std::string RegisterManager::acquire_scratch_reg() {
    std::string reg = find_free_register(SCRATCH_REGS);
    if (!reg.empty()) {
        registers[reg] = {IN_USE_SCRATCH, "scratch", false};
        return reg;
    }
    throw std::runtime_error("No scratch registers available.");
}