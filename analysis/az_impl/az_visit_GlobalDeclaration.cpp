#include "ASTAnalyzer.h"
#include <iostream>

/**
 * Handles global variable declarations.
 * Adds global variable names to the variable_definitions_ map for reliable detection.
 */
void ASTAnalyzer::visit(GlobalDeclaration& node) {
    for (const auto& pair : node.globals) {
        const std::string& name = pair.first;
        // Register this variable as belonging to the "Global" scope.
        variable_definitions_[name] = "Global";
        if (trace_enabled_) {
            std::cout << "[ANALYZER TRACE] Registered global variable: " << name << std::endl;
        }
    }
}