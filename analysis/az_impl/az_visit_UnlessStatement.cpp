#include "ASTAnalyzer.h"

// Visitor implementation for UnlessStatement nodes
void ASTAnalyzer::visit(UnlessStatement& node) {
    if (node.condition) {
        node.condition->accept(*this);
    }
    if (node.then_branch) {
        node.then_branch->accept(*this);
    }
}