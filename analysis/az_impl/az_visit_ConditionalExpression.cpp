#include "ASTAnalyzer.h"

// Visitor implementation for ConditionalExpression nodes
void ASTAnalyzer::visit(ConditionalExpression& node) {
    if (node.condition) {
        node.condition->accept(*this);
    }
    if (node.true_expr) {
        node.true_expr->accept(*this);
    }
    if (node.false_expr) {
        node.false_expr->accept(*this);
    }
}