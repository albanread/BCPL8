#include "ASTAnalyzer.h"

// Visitor implementation for DefaultStatement nodes
void ASTAnalyzer::visit(DefaultStatement& node) {
    // Visit the command in the default case, if present
    if (node.command) {
        node.command->accept(*this);
    }
}