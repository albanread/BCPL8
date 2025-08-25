#include "ASTAnalyzer.h"
#include "RuntimeManager.h"
#include <iostream>

/**
 * @brief Visits a FunctionCall node.
 * Tracks local and runtime function calls, and traverses arguments.
 */
void ASTAnalyzer::visit(FunctionCall& node) {


    if (auto* var_access = dynamic_cast<VariableAccess*>(node.function_expr.get())) {
        if (is_local_function(var_access->name)) {
            function_metrics_[current_function_scope_].num_local_function_calls++;
            if (trace_enabled_) std::cout << "[ANALYZER TRACE]   Detected call to local function: " << var_access->name << std::endl;
        } else if (RuntimeManager::instance().is_function_registered(var_access->name)) {
            function_metrics_[current_function_scope_].num_runtime_calls++;
            function_metrics_[current_function_scope_].accesses_globals = true; // <-- Flag global access for runtime call
            if (trace_enabled_) std::cout << "[ANALYZER TRACE]   Detected call to runtime function: " << var_access->name << ", Type: " << (get_runtime_function_type(var_access->name) == FunctionType::FLOAT ? "FLOAT" : "INTEGER") << std::endl;
        }
    }
    for (const auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}