#include "RegisterManager.h"
#include <iostream>

std::string RegisterManager::get_reg_for_cache_eviction(const std::string& routine_name) {
    std::string victim_reg;

    
    if (cache_lru_order_.size() < ROUTINE_CACHE_REGS.size()) {
        // Cache is not full, find an unused slot.

        for (const auto& reg : ROUTINE_CACHE_REGS) {
            if (cache_reg_to_routine_.find(reg) == cache_reg_to_routine_.end()) {
                victim_reg = reg;

                break;
            }
        }
    } else {
        // Cache is full, evict the least recently used register (the one at the back).
        victim_reg = cache_lru_order_.back();

        cache_lru_order_.pop_back();
        cache_reg_to_routine_.erase(victim_reg);
    }

    // Assign the new routine to the victim register and mark as most recently used.

    cache_reg_to_routine_[victim_reg] = routine_name;
    cache_lru_order_.push_front(victim_reg);

    // Mark the register as officially in use for a routine address.
    registers[victim_reg].status = IN_USE_ROUTINE_ADDR;

    // Print the current cache state


    return victim_reg;
}