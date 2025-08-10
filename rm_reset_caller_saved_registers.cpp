#include "RegisterManager.h"

void RegisterManager::reset_caller_saved_registers() {
    // Reset only caller-saved/scratch registers without affecting X19/X20 (routine cache registers)
    // or other callee-saved or special-purpose registers
    for (const auto& reg_name : SCRATCH_REGS) {
        if (registers.count(reg_name)) {
            registers[reg_name].status = FREE;
            registers[reg_name].bound_to = "";
            registers[reg_name].dirty = false;
            
            // Also remove any variable mappings to this register
            auto it = variable_to_reg_map.begin();
            while (it != variable_to_reg_map.end()) {
                if (it->second == reg_name) {
                    // Found a variable mapped to this register, remove it
                    variable_reg_lru_order_.remove(it->first);
                    it = variable_to_reg_map.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}