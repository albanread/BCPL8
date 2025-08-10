#include "RegisterManager.h"

void RegisterManager::release_scratch_reg(const std::string& reg_name) {
    if (registers.count(reg_name) && registers.at(reg_name).status == IN_USE_SCRATCH) {
        registers[reg_name] = {FREE, "", false};
    }
}