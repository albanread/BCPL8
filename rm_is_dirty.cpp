#include "RegisterManager.h"

bool RegisterManager::is_dirty(const std::string& reg_name) const {
    if (registers.count(reg_name)) {
        return registers.at(reg_name).dirty;
    }
    return false;
}