#include "../ASTAnalyzer.h"
#include <iostream>

// Helper to print boolean as "yes"/"no"
static const char* yesno(bool value) {
    return value ? "yes" : "no";
}

/**
 * @brief Prints a summary report of all function metrics, including register pressure.
 * 
 * This method generates a detailed report of all functions and routines analyzed,
 * including their types, parameter counts, local variable counts, and other metrics.
 */
void ASTAnalyzer::print_report() const {
    std::cout << "==== ASTAnalyzer Function Metrics Report ====" << std::endl;
    for (const auto& pair : function_metrics_) {
        const std::string& name = pair.first;
        const FunctionMetrics& metrics = pair.second;
        std::cout << "Function: " << name << std::endl;
        // Print the function type (int/float)
        auto type_it = function_return_types_.find(name);
        std::string type_str = (type_it != function_return_types_.end() && type_it->second == VarType::FLOAT) ? "float" : "int";
        std::cout << "  Type: " << type_str << std::endl;
        std::cout << "  Parameters: " << metrics.num_parameters << std::endl;
        std::cout << "  Integer Locals: " << metrics.num_variables << std::endl;
        std::cout << "  Float Locals: " << metrics.num_float_variables << std::endl;
        std::cout << "  Runtime Calls: " << metrics.num_runtime_calls << std::endl;
        std::cout << "  Local Function Calls: " << metrics.num_local_function_calls << std::endl;
        std::cout << "  Local Routine Calls: " << metrics.num_local_routine_calls << std::endl;
        std::cout << "  Vector Allocations: " << yesno(metrics.has_vector_allocations) << std::endl;
        std::cout << "  Accesses Globals: " << yesno(metrics.accesses_globals) << std::endl;
        std::cout << "  Max Live Variables (Register Pressure): " << metrics.max_live_variables << std::endl;
        std::cout << "--------------------------------------------" << std::endl;
    }
}