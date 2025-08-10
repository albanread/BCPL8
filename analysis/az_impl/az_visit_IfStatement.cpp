#include "ASTAnalyzer.h"

// Visitor implementation for IfStatement nodes
void ASTAnalyzer::visit(IfStatement& node) {
    if (node.condition) {
        node.condition->accept(*this);
    }
    if (node.then_branch) {
        node.then_branch->accept(*this);
    }
}