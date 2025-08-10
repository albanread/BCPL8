#include "ASTAnalyzer.h"

// Visitor implementation for VecAllocationExpression nodes
void ASTAnalyzer::visit(VecAllocationExpression& node) {
    // Mark that the current function/routine has vector allocations
    if (current_function_scope_ != "Global") {
        function_metrics_[current_function_scope_].has_vector_allocations = true;
    }
    // Visit the size expression if present
    if (node.size_expr) {
        node.size_expr->accept(*this);
    }
}