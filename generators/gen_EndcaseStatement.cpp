#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(EndcaseStatement& node) {
    debug_print("Visiting EndcaseStatement node.");
    // `ENDCASE` terminates a `SWITCHON` block. It usually doesn't generate code itself,
    // but marks the end of the `SWITCHON` structure.
    // Its functionality is typically integrated into the `SwitchonStatement` visitor.
    debug_print("EndcaseStatement node. No direct code emitted (handled by SwitchonStatement).");
}