#include "ASTAnalyzer.h"

// Visitor implementation for UntilStatement nodes
void ASTAnalyzer::visit(UntilStatement& node) {
    if (node.condition) {
        node.condition->accept(*this);
    }
    if (node.body) {
        node.body->accept(*this);
    }
}