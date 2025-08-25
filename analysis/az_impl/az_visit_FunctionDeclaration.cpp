#include "ASTAnalyzer.h"
#include <iostream>

void ASTAnalyzer::visit(FunctionDeclaration& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Visiting FunctionDeclaration: " << node.name << std::endl;

    // Synchronize SymbolTable scope
    if (symbol_table_) {
        symbol_table_->enterScope();
    }

    // --- START OF FIX ---
    // Reset FOR loop state for the new function scope.
    for_variable_unique_aliases_.clear();
    while (!active_for_loop_scopes_.empty()) {
        active_for_loop_scopes_.pop();
    }
    // --- END OF FIX ---

    // Save previous scopes to handle nested functions correctly.
    std::string previous_function_scope = current_function_scope_;
    std::string previous_lexical_scope = current_lexical_scope_;

    // Set both scopes to the function's name upon entry.
    current_function_scope_ = node.name;
    current_lexical_scope_ = node.name;

    function_metrics_[node.name].num_parameters = node.parameters.size();
    bool is_float_func = function_return_types_[node.name] == VarType::FLOAT;
    for (size_t i = 0; i < node.parameters.size(); ++i) {
        const auto& param = node.parameters[i];
        variable_definitions_[param] = node.name; // Defined in this function's scope.
        function_metrics_[node.name].parameter_indices[param] = i;

        // --- START OF FIX ---
        // If the function returns a float, assume its parameters are also floats.
        if (is_float_func) {
            function_metrics_[node.name].num_float_variables++;
            function_metrics_[node.name].variable_types[param] = VarType::FLOAT;
        } else {
            function_metrics_[node.name].num_variables++;
            function_metrics_[node.name].variable_types[param] = VarType::INTEGER;
        }
        // --- END OF FIX ---
    }

    if (node.body) {
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