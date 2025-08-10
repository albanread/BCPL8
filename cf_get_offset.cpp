#include "CallFrameManager.h"
#include <stdexcept>
#include <iostream>

int CallFrameManager::get_offset(const std::string& variable_name) const {
    debug_print("CallFrameManager::get_offset called for: '" + variable_name + "'");
    debug_print("Current variable_offsets_ map size: " + std::to_string(variable_offsets.size()));
    if (!is_prologue_generated) {
        fprintf(stderr, "[DEBUG] CallFrameManager::get_offset: variable_name = %s, is_prologue_generated = %d\n", variable_name.c_str(), (int)is_prologue_generated);
        throw std::runtime_error("Cannot get offset before prologue generation finalizes layout.");
    }
    auto it = variable_offsets.find(variable_name);
    if (it != variable_offsets.end()) {
        debug_print("CallFrameManager::get_offset: Found offset for '" + variable_name + "' = " + std::to_string(it->second));
        return it->second;
    }
    fprintf(stderr, "[DEBUG] CallFrameManager::get_offset: variable_name = %s not found in variable_offsets\n", variable_name.c_str());
    debug_print("CallFrameManager::get_offset: DID NOT find offset for '" + variable_name + "'");
    throw std::runtime_error("Variable '" + variable_name + "' not found in call frame.");
}
