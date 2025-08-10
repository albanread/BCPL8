#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(CompoundStatement& node) {
    debug_print("Visiting CompoundStatement node.");
    // A sequence of statements, often enclosed in `$( ... $)` or implicitly.
    // Simply visit each statement in order.
    for (const auto& stmt : node.statements) {
        if (stmt) {
            generate_statement_code(*stmt);
        }
    }
    debug_print("Finished visiting CompoundStatement node.");
}