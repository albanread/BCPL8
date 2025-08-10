#include "CallFrameManager.h"

int CallFrameManager::align_to_16(int size) const {
    return (size + 15) & ~15;
}
