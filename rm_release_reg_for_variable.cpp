#include "RegisterManager.h"

void RegisterManager::release_reg_for_variable(const std::string& variable_name) {
    if (variable_to_reg_map.count(variable_name)) {
        std::string reg = variable_to_reg_map[variable_name];
        if (registers.count(reg)) {
            // Spill logic is now handled by the acquire function when a register is needed.
            // Here, we just mark it as free.
            registers[reg] = {FREE, "", false};
            variable_to_reg_map.erase(variable_name);
            variable_reg_lru_order_.remove(reg);
        }
    }
}