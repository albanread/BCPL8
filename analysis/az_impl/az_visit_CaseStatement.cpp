#include "ASTAnalyzer.h"

// Implements ASTAnalyzer::visit for CaseStatement nodes.
void ASTAnalyzer::visit(CaseStatement& node) {
    // Visit the constant expression for the case, if present
    if (node.constant_expr) {
        node.constant_expr->accept(*this);
    }
    // Visit the command (body) of the case, if present
    if (node.command) {
        node.command->accept(*this);
    }
}