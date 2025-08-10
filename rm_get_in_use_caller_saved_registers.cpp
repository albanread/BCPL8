#include "RegisterManager.h"

std::vector<std::string> RegisterManager::get_in_use_caller_saved_registers() const {
    std::vector<std::string> in_use;
    for (const auto& reg_name : SCRATCH_REGS) {
        if (registers.count(reg_name) && registers.at(reg_name).status != FREE) {
            in_use.push_back(reg_name);
        }
    }
    return in_use;
}