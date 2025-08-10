#include "ASTAnalyzer.h"


// Visitor implementation for FloatVectorIndirection nodes
void ASTAnalyzer::visit(FloatVectorIndirection& node) {
    if (node.vector_expr) {
        node.vector_expr->accept(*this);
    }
    if (node.index_expr) {
        node.index_expr->accept(*this);
    }
}