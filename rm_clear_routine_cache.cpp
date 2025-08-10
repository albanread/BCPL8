#include "RegisterManager.h"

void RegisterManager::clear_routine_cache() {
    // Set the status of all cache registers back to FREE.
    for (const auto& reg : ROUTINE_CACHE_REGS) {
        if (registers.count(reg)) {
            registers[reg].status = FREE;
            registers[reg].bound_to = "";
        }
    }

    cache_reg_to_routine_.clear();
    cache_lru_order_.clear();
}