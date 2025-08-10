#include "CallFrameManager.h"

void CallFrameManager::mark_variable_as_float(const std::string& variable_name) {
    debug_print("CallFrameManager::mark_variable_as_float called for: '" + variable_name + "'");
    
    // Check if the variable exists
    if (!has_local(variable_name)) {
        debug_print("CallFrameManager::mark_variable_as_float: Variable '" + variable_name + "' not found.");
        return;
    }
    
    // Mark the variable as a floating-point type
    float_variables_[variable_name] = true;
    debug_print("CallFrameManager::mark_variable_as_float: Variable '" + variable_name + "' marked as float.");
}