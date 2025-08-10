#include "RegisterManager.h"

void RegisterManager::release_fp_register(const std::string& reg_name) {
    if (registers.count(reg_name)) {
        registers[reg_name].status = FREE;
        registers[reg_name].bound_to = "";
    }
}