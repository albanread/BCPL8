#include "CallFrameManager.h"

#include <algorithm> // Required for std::find_if

bool CallFrameManager::has_local(const std::string& variable_name) const {
    debug_print("CallFrameManager::has_local called for: '" + variable_name + "'");

    // Check the list of declared locals, which is populated BEFORE the prologue.
    auto it = std::find_if(local_declarations.begin(), local_declarations.end(),
        [&](const LocalVar& var) { return var.name == variable_name; });

    if (it != local_declarations.end()) {
        debug_print("CallFrameManager::has_local: Found '" + variable_name + "' in local_declarations.");
        return true;
    }
    
    // Also check saved registers, which are also assigned offsets.
    if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), variable_name) != callee_saved_registers_to_save.end()) {
        debug_print("CallFrameManager::has_local: Found '" + variable_name + "' in callee_saved_registers_to_save.");
        return true;
    }

    debug_print("CallFrameManager::has_local: DID NOT find '" + variable_name + "'.");
    return false;
}