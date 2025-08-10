#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(LabelDeclaration& node) {

    debug_print("Visiting LabelDeclaration node (Name: " + node.name + ").");

     // The label itself (e.g., "MyLabel:") has already been processed by the
     // CFGBuilderPass, which created a dedicated BasicBlock with a unique ID.
     // The NewCodeGenerator's main loop will emit that unique block ID as a label.
     // This visitor's only job is to generate the code for the statement
     // attached to the label declaration.

     if (node.command) {
         generate_statement_code(*node.command);
     }

    debug_print("Defined label '" + node.name + "'.");
}
