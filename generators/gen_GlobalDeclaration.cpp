#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(GlobalDeclaration& node) {
    debug_print("Visiting GlobalDeclaration node.");
    for (const auto& global_pair : node.globals) {
        const std::string& name = global_pair.first;
        debug_print("  Processing global vector: " + name);

        // Instruct the DataGenerator to allocate space for this global variable.
        // Since GLOBAL declares a vector (pointer), its initial value is 0 (nullptr).
        data_generator_.add_global_variable(name, nullptr); 
        debug_print("  Registered global variable '" + name + "' with the DataGenerator.");
    }
}