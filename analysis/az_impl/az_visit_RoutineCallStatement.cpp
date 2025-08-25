#include "ASTAnalyzer.h"
#include "RuntimeManager.h"
#include <iostream>

/**
 * @brief Visits a RoutineCallStatement node.
 * Tracks local and runtime routine calls, and traverses arguments.
 */
void ASTAnalyzer::visit(RoutineCallStatement& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Entered ASTAnalyzer::visit(RoutineCallStatement&) for node at " << &node << std::endl;
    if (auto* var_access = dynamic_cast<VariableAccess*>(node.routine_expr.get())) {
        // --- SETTYPE INTRINSIC LOGIC ---
        if (var_access->name == "SETTYPE" && node.arguments.size() == 2) {
            auto* target_var = dynamic_cast<VariableAccess*>(node.arguments[0].get());
            auto* type_const_expr = node.arguments[1].get();

            if (target_var && type_const_expr) {
                // Statically evaluate the type constant expression.
                bool has_value;
                int64_t type_val = evaluate_constant_expression(type_const_expr, &has_value);

                if (has_value) {
                    VarType new_type = static_cast<VarType>(type_val);

                    // Update the type in the current function's metrics and the symbol table.
                    if (!current_function_scope_.empty()) {
                        function_metrics_[current_function_scope_].variable_types[target_var->name] = new_type;
                    }
                    if (symbol_table_) {
                        symbol_table_->updateSymbolType(target_var->name, new_type);
                    }
                }
            }
            // Return immediately to prevent code generation for this pseudo-call.
            return;
        }
        // --- END SETTYPE LOGIC ---

        if (is_local_routine(var_access->name)) {
            function_metrics_[current_function_scope_].num_local_routine_calls++;
            if (trace_enabled_) std::cout << "[ANALYZER TRACE]   Detected call to local routine: " << var_access->name << std::endl;
        } else if (RuntimeManager::instance().is_function_registered(var_access->name)) {
            function_metrics_[current_function_scope_].num_runtime_calls++;
            function_metrics_[current_function_scope_].accesses_globals = true; // <-- Ensure X28 is loaded for runtime calls
            if (trace_enabled_) std::cout << "[ANALYZER TRACE]   Detected call to runtime function: " << var_access->name << ", Type: " << (get_runtime_function_type(var_access->name) == FunctionType::FLOAT ? "FLOAT" : "INTEGER") << std::endl;
        }
    }
    for (const auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}