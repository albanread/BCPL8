#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h" // Needed for analyzer_
#include <iostream>
#include <stdexcept>

// IMPORTANT: This file should ONLY contain the implementation of
// NewCodeGenerator::visit(RoutineDeclaration& node)
// The implementation of generate_function_like_code will go into NewCodeGenerator.cpp
// This file will now delegate to that common helper.

void NewCodeGenerator::visit(RoutineDeclaration& node) {
    debug_print("DEBUG: Visiting RoutineDeclaration node (Name: " + node.name + ").");


    // Delegate to the common helper method for all function-like code generation.
    // The body of a RoutineDeclaration is a Statement.
    generate_function_like_code(node.name, node.parameters, *node.body, false); // The 'false' indicates it's a non-value-returning routine.

    debug_print("Finished visiting RoutineDeclaration node."); //
}
