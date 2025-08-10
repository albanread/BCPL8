#include "../ASTAnalyzer.h"
#include "../AST.h"
#include <iostream>

/**
 * @brief Discovers all function and routine names in the program and initializes their metrics.
 * This is the first pass of the analysis phase.
 */
void ASTAnalyzer::first_pass_discover_functions(Program& program) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] --- PASS 1: Discovering all function definitions ---" << std::endl;
    for (const auto& decl : program.declarations) {
        if (auto* func_decl = dynamic_cast<FunctionDeclaration*>(decl.get())) {
            // --- FIX: Use a local copy of the function name ---
            std::string func_name = func_decl->name;
            local_function_names_.insert(func_name);
            function_metrics_[func_name] = FunctionMetrics();
            // Pre-allocate _temp0 through _temp3 as ANY
            for (int i = 0; i < 4; ++i) {
                std::string temp_name = "_temp" + std::to_string(i);
                function_metrics_[func_name].variable_types[temp_name] = VarType::ANY;
                function_metrics_[func_name].num_variables++;
            }

            if (func_decl->body && dynamic_cast<FloatValofExpression*>(func_decl->body.get())) {
                function_return_types_[func_name] = VarType::FLOAT;
                local_float_function_names_.insert(func_name);
            } else {
                function_return_types_[func_name] = VarType::INTEGER;
            }
        } else if (auto* routine_decl = dynamic_cast<RoutineDeclaration*>(decl.get())) {
            // --- FIX: Use a local copy of the routine name ---
            std::string routine_name = routine_decl->name;
            local_routine_names_.insert(routine_name);
            function_metrics_[routine_name] = FunctionMetrics();
            // Pre-allocate _temp0 through _temp3 as ANY
            for (int i = 0; i < 4; ++i) {
                std::string temp_name = "_temp" + std::to_string(i);
                function_metrics_[routine_name].variable_types[temp_name] = VarType::ANY;
                function_metrics_[routine_name].num_variables++;
            }
            function_return_types_[routine_name] = VarType::INTEGER;
        } else if (auto* let_decl = dynamic_cast<LetDeclaration*>(decl.get())) {
            if (let_decl->names.size() == 1 && let_decl->initializers.size() == 1) {
                // --- FIX: Use a local copy of the let name ---
                std::string let_name = let_decl->names[0];
                if (dynamic_cast<ValofExpression*>(let_decl->initializers[0].get())) {
                    local_function_names_.insert(let_name);
                    function_metrics_[let_name] = FunctionMetrics();
                    // Pre-allocate _temp0 through _temp3 as ANY
                    for (int i = 0; i < 4; ++i) {
                        std::string temp_name = "_temp" + std::to_string(i);
                        function_metrics_[let_name].variable_types[temp_name] = VarType::ANY;
                        function_metrics_[let_name].num_variables++;
                    }
                    function_return_types_[let_name] = VarType::INTEGER;
                } else if (dynamic_cast<FloatValofExpression*>(let_decl->initializers[0].get())) {
                    local_float_function_names_.insert(let_name);
                    function_metrics_[let_name] = FunctionMetrics();
                    // Pre-allocate _temp0 through _temp3 as ANY
                    for (int i = 0; i < 4; ++i) {
                        std::string temp_name = "_temp" + std::to_string(i);
                        function_metrics_[let_name].variable_types[temp_name] = VarType::ANY;
                        function_metrics_[let_name].num_variables++;
                    }
                    function_return_types_[let_name] = VarType::FLOAT;
                }
            }
        }
    }
}