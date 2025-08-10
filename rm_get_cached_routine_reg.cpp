#include "RegisterManager.h"
#include <iostream>
#include <iomanip>

std::string RegisterManager::get_cached_routine_reg(const std::string& routine_name) {


    // Correctly iterate through the map to find the register by routine name
    for (const auto& pair : cache_reg_to_routine_) {
        if (pair.second == routine_name) {
            std::string reg = pair.first;
            // CACHE HIT! Mark as most recently used.
            cache_lru_order_.remove(reg);
            cache_lru_order_.push_front(reg);
            return reg;
        }
    }
    return ""; // CACHE MISS!
}