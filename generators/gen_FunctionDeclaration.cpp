#include "../NewCodeGenerator.h"

#include "../analysis/ASTAnalyzer.h" // Needed for analyzer_



// IMPORTANT: This file should ONLY contain the implementation of
// NewCodeGenerator::visit(FunctionDeclaration& node)
// The implementation of generate_function_like_code will go into NewCodeGenerator.cpp
// This file will now delegate to that common helper.

void NewCodeGenerator::visit(FunctionDeclaration& node) {
    debug_print("Visiting FunctionDeclaration node (Name: " + node.name + ")."); //
    if (ASTAnalyzer::getInstance().get_function_metrics().count(node.name) > 0) {
        debug_print("Function metrics for " + node.name + ": runtime_calls=" + std::to_string(ASTAnalyzer::getInstance().get_function_metrics().at(node.name).num_runtime_calls) + //
                    ", local_function_calls=" + std::to_string(ASTAnalyzer::getInstance().get_function_metrics().at(node.name).num_local_function_calls) + //
                    ", local_routine_calls=" + std::to_string(ASTAnalyzer::getInstance().get_function_metrics().at(node.name).num_local_routine_calls)); //
    } else {
        debug_print("Function metrics for " + node.name + " not found.");
    }

    // Delegate to the common helper method for all function-like code generation.
    // The body of a FunctionDeclaration is an Expression.
    if (node.body) {
        generate_function_like_code(node.name, node.parameters, *node.body, true); // The 'true' indicates it's a value-returning function.
    } else {
        throw std::runtime_error("FunctionDeclaration: Body is null for function " + node.name);
    }

    debug_print("Finished visiting FunctionDeclaration node."); //
}
