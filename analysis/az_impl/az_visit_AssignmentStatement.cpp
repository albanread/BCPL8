#include "ASTAnalyzer.h"

// Visitor implementation for AssignmentStatement nodes
void ASTAnalyzer::visit(AssignmentStatement& node) {
    // --- Register Pressure Heuristic ---
    int current_live_count = node.lhs.size() + node.rhs.size();
    if (current_live_count > function_metrics_[current_function_scope_].max_live_variables) {
        function_metrics_[current_function_scope_].max_live_variables = current_live_count;
    }

    for (const auto& lhs : node.lhs) {
        if (lhs && lhs->getType() == ASTNode::NodeType::VariableAccessExpr) {
            auto* var = static_cast<VariableAccess*>(lhs.get());
            // Attribute variable to current function/routine if not already present
            if (variable_definitions_.find(var->name) == variable_definitions_.end()) {
                variable_definitions_[var->name] = current_function_scope_;
                if (current_function_scope_ != "Global") {
                    function_metrics_[current_function_scope_].num_variables++;
                }
            }
        }
        if (lhs) lhs->accept(*this);
    }
    for (const auto& rhs : node.rhs) {
        if (rhs) rhs->accept(*this);
    }
}