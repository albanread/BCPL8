#include "RegisterManager.h"
#include "CallFrameManager.h"
#include <stdexcept>

std::string RegisterManager::acquire_callee_saved_temp_reg(CallFrameManager& cfm) {
    std::string reg = find_free_register(VARIABLE_REGS);
    if (reg.empty()) {
        throw std::runtime_error("No free callee-saved registers available for temporary preservation.");
    }
    registers[reg].status = IN_USE_SCRATCH; // Or a new status if you prefer
    registers[reg].bound_to = "_persistent_temp_";
    cfm.force_save_register(reg);
    return reg;
}