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
        if (is_local_routine(var_access->name)) {
            function_metrics_[current_function_scope_].num_local_routine_calls++;
            if (trace_enabled_) std::cout << "[ANALYZER TRACE]   Detected call to local routine: " << var_access->name << std::endl;
        } else if (RuntimeManager::instance().is_function_registered(var_access->name)) {
            function_metrics_[current_function_scope_].num_runtime_calls++;
            if (trace_enabled_) std::cout << "[ANALYZER TRACE]   Detected call to runtime function: " << var_access->name << ", Type: " << (get_runtime_function_type(var_access->name) == FunctionType::FLOAT ? "FLOAT" : "INTEGER") << std::endl;
        }
    }
    for (const auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}