#include "CallFrameManager.h"
#include <stdexcept>
#include <algorithm>
#include <vector>

void CallFrameManager::force_save_x19_x20() {
    if (is_prologue_generated) {
        throw std::runtime_error("Cannot force saving registers after prologue is generated.");
    }
    debug_print("Checking if X19 and X20 need to be added to callee_saved_registers_to_save.");
    if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), "X19") == callee_saved_registers_to_save.end()) {
        callee_saved_registers_to_save.push_back("X19");
        debug_print("Added X19 to callee_saved_registers_to_save.");
    }
    if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), "X20") == callee_saved_registers_to_save.end()) {
        callee_saved_registers_to_save.push_back("X20");
        debug_print("Added X20 to callee_saved_registers_to_save.");
    }
    debug_print("Forced saving of X19 and X20 due to function call detection.");
}
