#include "ASTAnalyzer.h"

// Visitor implementation for TestStatement nodes
void ASTAnalyzer::visit(TestStatement& node) {
    // Visit the condition expression if present
    if (node.condition) {
        node.condition->accept(*this);
    }
    // Visit the then branch if present
    if (node.then_branch) {
        node.then_branch->accept(*this);
    }
    // Visit the else branch if present
    if (node.else_branch) {
        node.else_branch->accept(*this);
    }
}