#include "ASTAnalyzer.h"
#include <iostream>

void ASTAnalyzer::visit(RoutineDeclaration& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Visiting RoutineDeclaration: " << node.name << std::endl;

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

    if (node.body) {
        if (trace_enabled_) std::cout << "[ANALYZER TRACE] ASTAnalyzer::visit(RoutineDeclaration&) is traversing body for routine: " << node.name << std::endl;
        node.body->accept(*this);
    }

    // Restore previous scopes upon exit.
    current_function_scope_ = previous_function_scope;
    current_lexical_scope_ = previous_lexical_scope;
}