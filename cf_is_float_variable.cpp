#include "CallFrameManager.h"

bool CallFrameManager::is_float_variable(const std::string& variable_name) const {
    auto it = variable_types_.find(variable_name);
    return it != variable_types_.end() && it->second == VarType::FLOAT;
}