#include "ASTAnalyzer.h"

// Visitor implementation for StringAllocationExpression nodes
void ASTAnalyzer::visit(StringAllocationExpression& node) {
    // Visit the size expression if present
    if (node.size_expr) {
        node.size_expr->accept(*this);
    }
}