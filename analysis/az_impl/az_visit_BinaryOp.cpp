#include "ASTAnalyzer.h"

// Implements ASTAnalyzer::visit for BinaryOp nodes.
void ASTAnalyzer::visit(BinaryOp& node) {


    if (node.left) {
        node.left->accept(*this);
    }
    if (node.right) {
        node.right->accept(*this);
    }
}