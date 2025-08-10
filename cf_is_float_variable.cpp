#include "CallFrameManager.h"

bool CallFrameManager::is_float_variable(const std::string& variable_name) const {
    debug_print("CallFrameManager::is_float_variable called for: '" + variable_name + "'");
    
    // Check if the variable exists and is marked as a floating-point type
    auto it = float_variables_.find(variable_name);
    if (it != float_variables_.end() && it->second) {
        debug_print("CallFrameManager::is_float_variable: Variable '" + variable_name + "' is a float type.");
        return true;
    }
    
    debug_print("CallFrameManager::is_float_variable: Variable '" + variable_name + "' is not a float type.");
    return false;
}