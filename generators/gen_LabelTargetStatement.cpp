#include "NewCodeGenerator.h"
#include "AST.h"

void NewCodeGenerator::visit(LabelTargetStatement& node) {
    debug_print("Visiting LabelTargetStatement (Name: " + node.labelName + "). No code emitted.");
    // This visitor is now intentionally empty.
    // The label's position in the code has already been handled by the CFGBuilderPass,
    // which created a dedicated BasicBlock. The NewCodeGenerator's main loop
    // emits the unique label for that block (e.g., "START_Label_MyLabel2").
    // This visitor must not generate any additional instructions or labels.
}
