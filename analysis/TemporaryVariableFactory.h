#pragma once

#include <string>
#include "AST.h"
#include "SymbolTable.h"
#include "analysis/ASTAnalyzer.h"
#include "DataTypes.h"

// Factory for creating and registering temporary variables during optimization passes.
class TemporaryVariableFactory {
public:
    // Creates a new temporary variable, registers it, and returns its name.
    // - function_name: Name of the function in which the temp variable is created.
    // - var_type: The type of the temporary variable.
    // - symbol_table: Reference to the symbol table for registration.
    // - ast_analyzer: Reference to the ASTAnalyzer for updating metrics.
    std::string create(
        const std::string& function_name,
        VarType var_type,
        SymbolTable& symbol_table,
        ASTAnalyzer& ast_analyzer
    );

private:
    int temp_var_counter_ = 0;
};