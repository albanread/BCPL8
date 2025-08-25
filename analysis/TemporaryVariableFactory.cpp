#include "TemporaryVariableFactory.h"

std::string TemporaryVariableFactory::create(
    const std::string& function_name,
    VarType var_type,
    SymbolTable& symbol_table,
    ASTAnalyzer& ast_analyzer
) {
    // 1. Generate a new, unique name.
    std::string temp_name = "_opt_temp_" + std::to_string(temp_var_counter_++);

    // 2. Register the new variable in the Symbol Table.
    symbol_table.addSymbol(temp_name, SymbolKind::LOCAL_VAR, var_type);

    // 3. Update the function's metrics in the ASTAnalyzer.
    auto& metrics = ast_analyzer.get_function_metrics_mut().at(function_name);
    if (var_type == VarType::FLOAT) {
        metrics.num_float_variables++;
    } else {
        metrics.num_variables++;
    }
    metrics.variable_types[temp_name] = var_type;

    return temp_name;
}