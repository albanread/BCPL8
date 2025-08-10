#include "Encoder.h"

Instruction Encoder::create_csetm_ne(const std::string& xd) {
    return create_csetm(xd, "NE");
}
