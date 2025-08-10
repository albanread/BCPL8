#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(StaticDeclaration& node) {
    debug_print("Visiting StaticDeclaration node (Name: " + node.name + ").");
    // Static declarations are similar to global declarations in terms of memory allocation,
    // but their scope is typically limited to the compilation unit or function.
    // In terms of code generation, they generally result in data being emitted to the .data/.bss section.

    // Define a label for the static variable. Static variables are often
    // internal to the object file, not exposed externally by default.
    std::string static_label = label_manager_.create_label();
    instruction_stream_.define_label(static_label);

    if (node.initializer) {
        // Similar to global initializers, static initializers need to be handled
        // as data emission. For now, only simple literals.
        if (node.initializer->is_literal()) {
            if (NumberLiteral* num_lit = dynamic_cast<NumberLiteral*>(node.initializer.get())) {
                if (num_lit->literal_type == NumberLiteral::LiteralType::Integer) {
                    instruction_stream_.add_data64(num_lit->int_value, static_label);
                    debug_print("Emitted static integer variable '" + node.name + "' with value " + std::to_string(num_lit->int_value) + ".");
                } else {
                    throw std::runtime_error("StaticDeclaration: Float initializers not yet supported.");
                }
            } else {
                throw std::runtime_error("StaticDeclaration: Non-numeric literal initializers not yet supported.");
            }
        } else {
            // For non-literal static initializers, typically zero-initialized
            instruction_stream_.add_data64(0, static_label);
            debug_print("Emitted static variable '" + node.name + "' with zero initializer (non-literal not supported yet).");
        }
    } else {
        // No initializer, default to zero or a BSS entry.
        instruction_stream_.add_data64(0, static_label); // Emit a 64-bit zero word for BSS-like behavior
        debug_print("Emitted static variable '" + node.name + "' with default zero initializer.");
    }

    // Static variables also need to be accessible. They might be added to the current
    // scope if they're function-local statics, or a global symbol table if they're file-scope.
    // For now, we'll assume file-scope and they'll be looked up by their label.
    // They are not "local" to a call frame, so `is_local_variable` should return false.
    // A separate map for static variable labels/addresses would be ideal.
    debug_print("Finished visiting StaticDeclaration node.");
}