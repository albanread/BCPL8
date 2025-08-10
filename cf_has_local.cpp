#include "CallFrameManager.h"

bool CallFrameManager::has_local(const std::string& variable_name) const {
    debug_print("CallFrameManager::has_local called for: '" + variable_name + "'");
    debug_print("Current variable_offsets_ map size: " + std::to_string(variable_offsets.size()));
    if (variable_offsets.count(variable_name) > 0) {
        debug_print("CallFrameManager::has_local: Found '" + variable_name + "'.");
        return true;
    }
    debug_print("CallFrameManager::has_local: DID NOT find '" + variable_name + "'.");
    return false;
}
