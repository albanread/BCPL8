#include "ASTAnalyzer.h"

// Visitor implementation for StringStatement nodes
void ASTAnalyzer::visit(StringStatement& node) {
    // Visit the size expression if present
    if (node.size_expr) {
        node.size_expr->accept(*this);
    }
}