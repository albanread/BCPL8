#include "CallFrameManager.h"
#include <stdexcept>

void CallFrameManager::add_local(const std::string& variable_name, int size_in_bytes) {
    if (is_prologue_generated) {
        throw std::runtime_error("Cannot add locals after prologue is generated.");
    }
    if (size_in_bytes % 8 != 0) {
        throw std::runtime_error("Local variable size must be a multiple of 8 bytes.");
    }
    local_declarations.push_back(LocalVar(variable_name, size_in_bytes));
    locals_total_size += size_in_bytes;
}


void CallFrameManager::add_parameter(const std::string& name) {
    add_local(name, 8);
}

