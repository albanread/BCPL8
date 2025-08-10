#include "RegisterManager.h"

std::vector<std::pair<std::string, std::string>> RegisterManager::get_dirty_variable_registers() const {
    std::vector<std::pair<std::string, std::string>> dirty_regs;
    for (const auto& pair : registers) {
        const RegisterInfo& info = pair.second;
        if (info.status == IN_USE_VARIABLE && info.dirty) {
            dirty_regs.push_back({pair.first, info.bound_to}); // {register name, variable name}
        }
    }
    return dirty_regs;
}