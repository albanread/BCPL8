#include "ASTAnalyzer.h"

// Implements ASTAnalyzer::visit for CompoundStatement nodes.
void ASTAnalyzer::visit(CompoundStatement& node) {
    for (const auto& stmt : node.statements) {
        if (stmt) {
            stmt->accept(*this);
        }
    }
}