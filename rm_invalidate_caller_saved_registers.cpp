#include "RegisterManager.h"

void RegisterManager::invalidate_caller_saved_registers() {
    static const std::vector<std::string> CALLER_SAVED_GP = {
        "X0", "X1", "X2", "X3", "X4", "X5", "X6", "X7",
        "X8", "X9", "X10", "X11", "X12", "X13", "X14", "X15", "X16", "X17"
    };

    for(const auto& reg_name : CALLER_SAVED_GP) {
        if (registers.count(reg_name) && registers[reg_name].status == IN_USE_VARIABLE) {
            std::string var_name = registers[reg_name].bound_to;
            variable_to_reg_map.erase(var_name);
            variable_reg_lru_order_.remove(reg_name);
            registers[reg_name] = {FREE, "", false};
        }
    }
}