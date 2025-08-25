#include "ASTAnalyzer.h"
#include "../../DataTypes.h"

// Visitor implementation for AssignmentStatement nodes
void ASTAnalyzer::visit(AssignmentStatement& node) {
    // Visit all RHS expressions first (to analyze subexpressions)
    for (const auto& rhs : node.rhs) {
        if (rhs) rhs->accept(*this);
    }

    // For each LHS variable, infer the type from the corresponding RHS
    size_t count = std::min(node.lhs.size(), node.rhs.size());
    for (size_t i = 0; i < count; ++i) {
        auto* var = dynamic_cast<VariableAccess*>(node.lhs[i].get());
        if (var) {
            VarType rhs_type = infer_expression_type(node.rhs[i].get());
            // Update the variable type in function_metrics_
            if (!current_function_scope_.empty()) {
                function_metrics_[current_function_scope_].variable_types[var->name] = rhs_type;
            }
            // Attribute variable to current function/routine if not already present
            if (variable_definitions_.find(var->name) == variable_definitions_.end()) {
                variable_definitions_[var->name] = current_function_scope_;
                if (current_function_scope_ != "Global") {
                    function_metrics_[current_function_scope_].num_variables++;
                }
                // Always add the variable to the SymbolTable if not present in the current scope
                if (symbol_table_) {
                    symbol_table_->addSymbol(var->name, SymbolKind::LOCAL_VAR, rhs_type);
                }
            }
        }

        // --- NEW: Prevent assignment to HD/TL of MANIFESTLIST (const list types) ---
        if (auto* un_op = dynamic_cast<UnaryOp*>(node.lhs[i].get())) {
            if (un_op->op == UnaryOp::Operator::HeadOf ||
                un_op->op == UnaryOp::Operator::TailOf) {
                VarType list_type = infer_expression_type(un_op->operand.get());
                if (is_const_list_type(list_type)) {
                    std::cerr << "Semantic Error: Cannot assign to a part of a read-only MANIFESTLIST." << std::endl;
                    // Optionally: set an error flag or throw
                }
            }
        }

        if (node.lhs[i]) node.lhs[i]->accept(*this);
    }

    // Visit any remaining LHS expressions (in case of destructuring or side effects)
    for (size_t i = count; i < node.lhs.size(); ++i) {
        if (node.lhs[i]) node.lhs[i]->accept(*this);
    }
}