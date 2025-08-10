#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(SwitchonStatement& node) {
    debug_print("Visiting SwitchonStatement node (NOTE: branching is handled by block epilogue).");
    // All branching logic has been moved to generate_block_epilogue to align with the
    // CFG-driven code generation strategy. This visitor is now intentionally left empty
    // to prevent the generation of duplicate comparison and branch instructions, which
    // was causing CASE blocks to execute multiple times.
}
