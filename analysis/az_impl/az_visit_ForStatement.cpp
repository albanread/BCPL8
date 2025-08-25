#include "ASTAnalyzer.h"
#include <iostream>
#include <map>
#include <string>

/**
 * @brief Visits a ForStatement node.
 * Handles unique naming for loop variables, manages scope, and traverses the loop body.
 */
void ASTAnalyzer::visit(ForStatement& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Visiting ForStatement for variable: " << node.loop_variable << std::endl;

    // --- Determine the unique name for the loop variable itself ---
    std::string canonical_for_var_name;
    if (for_variable_unique_aliases_.count(node.loop_variable)) {
        canonical_for_var_name = for_variable_unique_aliases_[node.loop_variable];
        if (trace_enabled_) {
            std::cout << "[ANALYZER TRACE]   Reusing unique loop var '" << canonical_for_var_name
                      << "' for original '" << node.loop_variable << "' (previously declared FOR var)." << std::endl;
        }
    } else {
        canonical_for_var_name = node.loop_variable + "_for_var_" + std::to_string(for_loop_var_counter_++);
        for_variable_unique_aliases_[node.loop_variable] = canonical_for_var_name;
        if (current_function_scope_ != "Global") {
            variable_definitions_[canonical_for_var_name] = current_function_scope_;
            function_metrics_[current_function_scope_].num_variables++;
            // Register the type of the new loop variable. It's always an integer.
            function_metrics_[current_function_scope_].variable_types[canonical_for_var_name] = VarType::INTEGER;
            if (trace_enabled_) {
                std::cout << "[ANALYZER TRACE]   Created NEW unique loop var '" << canonical_for_var_name
                          << "' for original '" << node.loop_variable << "'. Defined for stack space. Incremented var count." << std::endl;
            }
        } else {
            variable_definitions_[canonical_for_var_name] = "Global";
            if (trace_enabled_) {
                std::cout << "[ANALYZER TRACE]   Created NEW unique global loop var '" << canonical_for_var_name
                          << "' for original '" << node.loop_variable << "'. Defined as global." << std::endl;
            }
        }
    }
    node.unique_loop_variable_name = canonical_for_var_name;

    // --- Manage active_for_loop_scopes_ for nested FOR loops ---
    std::map<std::string, std::string> current_for_scope_map;
    current_for_scope_map[node.loop_variable] = node.unique_loop_variable_name;
    active_for_loop_scopes_.push(current_for_scope_map);
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Pushed FOR loop scope for '" << node.loop_variable << "' -> '" << node.unique_loop_variable_name << "'. Stack size: " << active_for_loop_scopes_.size() << std::endl;

    // --- Step and End variables (unique per loop instance) ---
    node.unique_step_variable_name = node.unique_loop_variable_name + "_step_inst_" + std::to_string(for_loop_instance_suffix_counter);
    variable_definitions_[node.unique_step_variable_name] = current_function_scope_;
    if (current_function_scope_ != "Global") {
        function_metrics_[current_function_scope_].num_variables++;
    }
    if (trace_enabled_) {
        std::cout << "[ANALYZER TRACE]   Created backing var for step: '" << node.unique_step_variable_name << "'" << std::endl;
    }

    node.unique_end_variable_name = node.unique_loop_variable_name + "_end_inst_" + std::to_string(for_loop_instance_suffix_counter);
    variable_definitions_[node.unique_end_variable_name] = current_function_scope_;
    if (current_function_scope_ != "Global") {
        function_metrics_[current_function_scope_].num_variables++;
    }
    if (trace_enabled_) {
        std::cout << "[ANALYZER TRACE]   Created backing var for hoisted end value: '" << node.unique_end_variable_name << "'" << std::endl;
    }

    for_loop_instance_suffix_counter++;
    for_statements_[node.unique_loop_variable_name] = &node;

    if (node.start_expr) node.start_expr->accept(*this);
    if (node.end_expr) node.end_expr->accept(*this);
    if (node.step_expr) node.step_expr->accept(*this);

    // Visit the body after pushing the scope
    if (node.body) node.body->accept(*this);

    // Pop the scope after visiting the body
    active_for_loop_scopes_.pop();
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Popped FOR loop scope. Stack size: " << active_for_loop_scopes_.size() << std::endl;
}