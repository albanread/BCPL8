#include "RegisterManager.h"

void RegisterManager::mark_dirty(const std::string& reg_name, bool is_dirty) {
    if (registers.count(reg_name)) {
        registers.at(reg_name).dirty = is_dirty;
    }
}