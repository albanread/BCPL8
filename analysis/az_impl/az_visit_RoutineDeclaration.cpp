#include "ASTAnalyzer.h"
#include <iostream>

void ASTAnalyzer::visit(RoutineDeclaration& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Visiting RoutineDeclaration: " << node.name << std::endl;

    // Synchronize SymbolTable scope
    if (symbol_table_) {
        symbol_table_->enterScope();
    }

    // --- START OF FIX ---
    // Reset FOR loop state for the new routine scope.
    for_variable_unique_aliases_.clear();
    while (!active_for_loop_scopes_.empty()) {
        active_for_loop_scopes_.pop();
    }
    // --- END OF FIX ---

    // Save previous scopes.
    std::string previous_function_scope = current_function_scope_;
    std::string previous_lexical_scope = current_lexical_scope_;

    // Set both scopes to the routine's name upon entry.
    current_function_scope_ = node.name;
    current_lexical_scope_ = node.name;

    function_metrics_[node.name].num_parameters = node.parameters.size();
    bool is_float_func = function_return_types_[node.name] == VarType::FLOAT;
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        const auto& param = node.parameters[i];
        variable_definitions_[param] = node.name;
        function_metrics_[node.name].parameter_indices[param] = i;

        // --- START OF FIX ---
        // If the routine is marked as float-related, assume its parameters are floats.
        if (is_float_func) {
            function_metrics_[node.name].num_float_variables++;
            function_metrics_[node.name].variable_types[param] = VarType::FLOAT;
        } else {
            function_metrics_[node.name].num_variables++;
            function_metrics_[node.name].variable_types[param] = VarType::INTEGER;
        }
        // --- END OF FIX ---
    }

    // --- TWO-PASS ANALYSIS IMPLEMENTATION ---
    // First pass: collect all variable types from LetDeclarations in the routine body (if CompoundStatement)
    if (node.body) {
        CompoundStatement* compound = dynamic_cast<CompoundStatement*>(node.body.get());
        if (compound) {
            for (const auto& stmt : compound->statements) {
                if (auto* let_decl = dynamic_cast<LetDeclaration*>(stmt.get())) {
                    this->visit(*let_decl); // This will populate variable types
                }
            }
        }
    }

    // Second pass: analyze all statements as usual
    if (node.body) {
        if (trace_enabled_) std::cout << "[ANALYZER TRACE] ASTAnalyzer::visit(RoutineDeclaration&) is traversing body for routine: " << node.name << std::endl;
        node.body->accept(*this);
    }

    // Restore previous scopes upon exit.
    current_function_scope_ = previous_function_scope;
    current_lexical_scope_ = previous_lexical_scope;

    // Synchronize SymbolTable scope
    if (symbol_table_) {
        symbol_table_->exitScope();
    }
}