#include "ASTAnalyzer.h"

// Implements ASTAnalyzer::visit for BinaryOp nodes.
void ASTAnalyzer::visit(BinaryOp& node) {
    // --- Register Pressure Heuristic ---
    int current_live_count = (node.left ? 1 : 0) + (node.right ? 1 : 0);
    if (current_live_count > function_metrics_[current_function_scope_].max_live_variables) {
        function_metrics_[current_function_scope_].max_live_variables = current_live_count;
    }

    if (node.left) {
        node.left->accept(*this);
    }
    if (node.right) {
        node.right->accept(*this);
    }
}