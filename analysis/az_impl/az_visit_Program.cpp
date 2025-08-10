#include "ASTAnalyzer.h"
#include <iostream>

/**
 * @brief Visits the root Program node of the AST.
 * Initializes global scope and traverses all top-level declarations and statements.
 */
void ASTAnalyzer::visit(Program& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Visiting Program." << std::endl;
    current_function_scope_ = "Global";
    for_loop_var_counter_ = 0;
    for_variable_unique_aliases_.clear();
    while (!active_for_loop_scopes_.empty()) {
        active_for_loop_scopes_.pop();
    }
    for_loop_instance_suffix_counter = 0;
    for (const auto& decl : node.declarations) {
        if (decl) decl->accept(*this);
    }
    for (size_t i = 0; i < node.statements.size(); ++i) {
        if (node.statements[i]) {
            std::cout << "[ANALYZER TRACE] Program dispatching statement " << i << " of type " << static_cast<int>(node.statements[i]->getType()) << std::endl;
            node.statements[i]->accept(*this);
        }
    }
}