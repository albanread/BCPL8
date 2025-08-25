#include "ASTAnalyzer.h"
#include <iostream>

void ASTAnalyzer::visit(VariableAccess& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Visiting VariableAccess: " << node.name << std::endl;

    // Handle FOR loop variable renaming first.
    std::string effective_name = get_effective_variable_name(node.name);
    if (effective_name != node.name) {
        node.name = effective_name;
    }

    // Use the SymbolTable to check if this is a global variable.
    Symbol symbol;
    if (symbol_table_ && symbol_table_->lookup(node.name, symbol)) {
        // If the symbol is a GLOBAL_VAR, it's a global access.
        if (symbol.kind == SymbolKind::GLOBAL_VAR) {
            if (current_function_scope_ != "Global") {
                function_metrics_[current_function_scope_].accesses_globals = true;
                if (trace_enabled_) {
                    std::cout << "[ANALYZER TRACE]   Marked function '" << current_function_scope_ << "' as accessing global '" << node.name << "'." << std::endl;
                }
            }
        }
    } else {
        // Always add the variable to the SymbolTable if not present in the current scope
        if (symbol_table_) {
            symbol_table_->addSymbol(node.name, SymbolKind::LOCAL_VAR, VarType::UNKNOWN);
        }
        if (variable_definitions_.find(node.name) == variable_definitions_.end()) {
            // If it's a new, undeclared variable, define it in the current lexical scope.
            variable_definitions_[node.name] = current_lexical_scope_;
            if (current_function_scope_ != "Global") {
                function_metrics_[current_function_scope_].num_variables++;
            }
        }
    }
}