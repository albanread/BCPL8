#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(ManifestDeclaration& node) {
    debug_print("Visiting ManifestDeclaration node (Name: " + node.name + ", Value: " + std::to_string(node.value) + ").");
    // Manifests are compile-time constants. They don't generate code or allocate memory.
    // Their values are substituted during semantic analysis or by the code generator directly.
    // No code emission is typically needed here.
    // They should ideally be resolved in a symbol table lookup before code generation.
    debug_print("Manifest '" + node.name + "' resolved to value " + std::to_string(node.value) + ". No code emitted.");
}