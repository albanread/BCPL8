#include "ASTAnalyzer.h"
#include "AST.h"

// Visitor implementation for VectorAccess nodes
void ASTAnalyzer::visit(VectorAccess& node) {
    // Visit the vector expression if present
    if (node.vector_expr) {
        node.vector_expr->accept(*this);
    }
    // Visit the index expression if present
    if (node.index_expr) {
        node.index_expr->accept(*this);
    }
}