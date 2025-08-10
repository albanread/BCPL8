#include "ASTAnalyzer.h"
#include <iostream>
#include <sstream>

void ASTAnalyzer::visit(BlockStatement& node) {
    // Save the current lexical scope before creating a new one.
    std::string previous_lexical_scope = current_lexical_scope_;

    // Create a new, unique name for this block's lexical scope.
    std::ostringstream block_name_ss;
    block_name_ss << previous_lexical_scope << "_block_" << variable_definitions_.size();
    current_lexical_scope_ = block_name_ss.str();

    if (trace_enabled_) {
        std::cout << "[ANALYZER TRACE] Entering block scope: " << current_lexical_scope_
                  << " (Function scope remains: " << current_function_scope_ << ")" << std::endl;
        std::cout << "[ANALYZER TRACE] BlockStatement: Traversing " << node.statements.size() << " statements." << std::endl;
    }

    // Traverse declarations and statements within this new lexical scope.
    for (const auto& decl : node.declarations) if (decl) decl->accept(*this);
    for (size_t i = 0; i < node.statements.size(); ++i) {
        if (node.statements[i]) {
            if (trace_enabled_) {
                std::cout << "[ANALYZER TRACE] BlockStatement: Calling accept on statement " << i << " of type " << static_cast<int>(node.statements[i]->getType()) << std::endl;
            }
            node.statements[i]->accept(*this);
        }
    }

    // Restore the previous lexical scope upon exiting the block.
    current_lexical_scope_ = previous_lexical_scope;
    if (trace_enabled_) {
        std::cout << "[ANALYZER TRACE] Exiting block scope, returning to: " << current_lexical_scope_ << std::endl;
    }
}