#include "CallFrameManager.h"
#include <stdexcept>

int CallFrameManager::get_x29_spill_slot_offset() const {
    if (!is_prologue_generated) {
        throw std::runtime_error("Cannot get spill slot offset before prologue generation.");
    }
    return x29_spill_slot_offset;
}
