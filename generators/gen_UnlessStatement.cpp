#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(UnlessStatement& node) {
    debug_print("Visiting UnlessStatement node (NOTE: branching is handled by block epilogue).");
    // No branching logic here; only evaluate condition if needed elsewhere.
}