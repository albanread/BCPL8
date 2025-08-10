#include "RegisterManager.h"

std::vector<std::string> RegisterManager::get_in_use_fp_callee_saved_registers() const {
    std::vector<std::string> used;
    for (const auto& reg : FP_VARIABLE_REGS) {
        if (registers.at(reg).status == IN_USE_VARIABLE) {
            used.push_back(reg);
        }
    }
    return used;
}