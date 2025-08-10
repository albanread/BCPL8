#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include "Symbol.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <stack>
#include <memory>



// The SymbolTable class manages scopes and symbols.
// It uses a stack of maps to handle nested scopes.
class SymbolTable {
public:
    SymbolTable();
    ~SymbolTable() = default;
    
    // Scope management
    void enterScope();
    void exitScope();
    int currentScopeLevel() const { return current_scope_level_; }
    
    // Symbol manipulation
    bool addSymbol(const Symbol& symbol);
    bool addSymbol(const std::string& name, SymbolKind kind, VarType type);
    
    // Symbol lookup
    bool lookup(const std::string& name, Symbol& symbol) const;
    bool lookupInCurrentScope(const std::string& name, Symbol& symbol) const;
    
    // Type information
    void setSymbolType(const std::string& name, VarType type);
    
    // Location management
    void setSymbolStackLocation(const std::string& name, int offset);
    void setSymbolDataLocation(const std::string& name, size_t offset);
    void setSymbolAbsoluteValue(const std::string& name, int64_t value);
    
    // Function parameter management
    void addParameter(const std::string& function_name, const std::string& param_name, VarType type);
    std::vector<std::string> getFunctionParameters(const std::string& function_name) const;
    
    // Context management
    void setCurrentFunction(const std::string& function_name);
    std::string getCurrentFunction() const { return current_function_; }
    
    // Iteration and utility
    std::vector<Symbol> getAllSymbols() const;
    std::vector<Symbol> getSymbolsInScope(int scope_level) const;
    std::vector<Symbol> getGlobalSymbols() const;
    std::vector<Symbol> getLocalSymbols() const;
    
    // Debug utilities
    void dumpTable() const;
    std::string toString() const;
    
private:
    // The stack of scope maps. Each map represents symbols in a single scope.
    std::vector<std::unordered_map<std::string, Symbol>> scope_stack_;
    
    // Current scope level (0 = global)
    int current_scope_level_ = 0;
    
    // Track the current function for parameter management
    std::string current_function_;
    
    // Map of function names to their parameter lists
    std::unordered_map<std::string, std::vector<std::string>> function_parameters_;
};


#endif // SYMBOL_TABLE_H