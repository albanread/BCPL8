#include "ASTAnalyzer.h"

// Visitor implementation for RepeatStatement nodes
void ASTAnalyzer::visit(RepeatStatement& node) {
    // Visit the body of the repeat loop, if present
    if (node.body) {
        node.body->accept(*this);
    }
    // Visit the condition of the repeat loop, if present
    if (node.condition) {
        node.condition->accept(*this);
    }
}