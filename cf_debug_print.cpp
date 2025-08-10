#include "CallFrameManager.h"
#include <iostream>

void CallFrameManager::debug_print(const std::string& message) const {
    if (debug_enabled) {
        std::cerr << "// [CFM-DEBUG] " << message << std::endl;
    }
}
