#include "ASTAnalyzer.h"
#include <iostream>


void ASTAnalyzer::reset_state() {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Resetting state." << std::endl;
    function_metrics_.clear();
    variable_definitions_.clear();
    current_function_scope_ = "Global";
    // --- FIX ---
    // Changed 'current_scope_name_' to 'current_lexical_scope_' to match the variable declared in the header file.
    current_lexical_scope_ = "Global";
    // --- END FIX ---
    local_function_names_.clear();
    local_routine_names_.clear();
    for_loop_var_counter_ = 0;
    for_variable_unique_aliases_.clear();
    while (!active_for_loop_scopes_.empty()) {
        active_for_loop_scopes_.pop();
    }
    for_loop_instance_suffix_counter = 0;
}