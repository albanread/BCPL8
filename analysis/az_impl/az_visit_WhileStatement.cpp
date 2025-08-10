#include "ASTAnalyzer.h"
#include <iostream>

// Visitor implementation for WhileStatement nodes
void ASTAnalyzer::visit(WhileStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}