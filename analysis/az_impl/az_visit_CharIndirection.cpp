#include "ASTAnalyzer.h"


// Visitor implementation for CharIndirection nodes
void ASTAnalyzer::visit(CharIndirection& node) {
    if (node.string_expr) {
        node.string_expr->accept(*this);
    }
    if (node.index_expr) {
        node.index_expr->accept(*this);
    }
}