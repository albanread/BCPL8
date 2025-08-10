#include "ASTAnalyzer.h"

// Visitor implementation for UnaryOp nodes
void ASTAnalyzer::visit(UnaryOp& node) {
    if (node.operand) {
        node.operand->accept(*this);
    }
}