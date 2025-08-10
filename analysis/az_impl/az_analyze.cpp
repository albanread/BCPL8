#include "../ASTAnalyzer.h"
#include "../../SymbolTable.h"
#include <iostream>

/**
 * @brief Performs semantic analysis on the given program AST.
 * This method resets the analyzer state, discovers all functions/routines,
 * and traverses the AST to collect metrics and semantic information.
 */
void ASTAnalyzer::analyze(Program& program, SymbolTable* symbol_table) {
    // Store the symbol table reference if provided
    symbol_table_ = symbol_table;
    
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Starting analysis..." << std::endl;
    reset_state();
    first_pass_discover_functions(program);
    program.accept(*this);
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Analysis complete." << std::endl;
}

// Keep the original method for backward compatibility
// This function is already correctly defined in lines 9-17 above
// So we're removing this redundant definition that doesn't match the declaration