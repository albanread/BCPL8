#include "RegisterManager.h"

std::vector<std::string> RegisterManager::get_in_use_callee_saved_registers() const {
    static const std::vector<std::string> CALLEE_SAVED_REGS = {
        "X19", "X20", "X21", "X22", "X23", "X24", "X25", "X26", "X27", "X28"
    };

    std::vector<std::string> in_use;
    for (const auto& reg_name : CALLEE_SAVED_REGS) {
        if (registers.count(reg_name) && registers.at(reg_name).status != FREE) {
            in_use.push_back(reg_name);
        }
    }
    return in_use;
}