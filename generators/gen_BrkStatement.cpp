#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "RegisterManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(BrkStatement& node) {
    debug_print("Visiting BrkStatement node.");
    // `BRK` instruction for debugging/software breakpoints.
    emit(Encoder::create_brk(0)); // Emit a BRK instruction with immediate 0
    debug_print("Emitted BRK instruction.");
}