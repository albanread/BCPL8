#include "SymbolTable.h"
#include <iostream>
#include <sstream>



SymbolTable::SymbolTable() {
    // Initialize with global scope (level 0)
    scope_stack_.push_back(std::unordered_map<std::string, Symbol>());
}

void SymbolTable::enterScope() {
    // Create a new scope
    scope_stack_.push_back(std::unordered_map<std::string, Symbol>());
    current_scope_level_++;
}

void SymbolTable::exitScope() {
    if (current_scope_level_ > 0) {
        // Remove the current scope
        scope_stack_.pop_back();
        current_scope_level_--;
    } else {
        // Cannot exit global scope
        std::cerr << "Warning: Attempting to exit global scope" << std::endl;
    }
}

bool SymbolTable::addSymbol(const Symbol& symbol) {
    // Check if symbol already exists in current scope
    auto& current_scope = scope_stack_[current_scope_level_];
    if (current_scope.find(symbol.name) != current_scope.end()) {
        return false; // Symbol already exists in current scope
    }
    
    // Add symbol to current scope
    current_scope.emplace(symbol.name, symbol);
    return true;
}

bool SymbolTable::addSymbol(const std::string& name, SymbolKind kind, VarType type) {
    Symbol new_symbol(name, kind, type, current_scope_level_);
    return addSymbol(new_symbol);
}

bool SymbolTable::lookup(const std::string& name, Symbol& symbol) const {
    // Start from current scope and work backwards
    for (int level = current_scope_level_; level >= 0; level--) {
        const auto& scope = scope_stack_[level];
        auto it = scope.find(name);
        if (it != scope.end()) {
            symbol = it->second;
            return true;
        }
    }
    
    // Symbol not found
    return false;
}

bool SymbolTable::lookupInCurrentScope(const std::string& name, Symbol& symbol) const {
    const auto& current_scope = scope_stack_[current_scope_level_];
    auto it = current_scope.find(name);
    
    if (it != current_scope.end()) {
        symbol = it->second;
        return true;
    }
    
    // Symbol not found in current scope
    return false;
}

void SymbolTable::setSymbolType(const std::string& name, VarType type) {
    Symbol symbol("", SymbolKind::LOCAL_VAR, VarType::UNKNOWN, 0);
    if (lookup(name, symbol)) {
        // Find the scope level where this symbol exists
        for (int level = current_scope_level_; level >= 0; level--) {
            auto& scope = scope_stack_[level];
            auto it = scope.find(name);
            if (it != scope.end()) {
                it->second.type = type;
                break;
            }
        }
    }
}

void SymbolTable::setSymbolStackLocation(const std::string& name, int offset) {
    Symbol symbol("", SymbolKind::LOCAL_VAR, VarType::UNKNOWN, 0);
    if (lookup(name, symbol)) {
        // Find the scope level where this symbol exists
        for (int level = current_scope_level_; level >= 0; level--) {
            auto& scope = scope_stack_[level];
            auto it = scope.find(name);
            if (it != scope.end()) {
                it->second.location = SymbolLocation::stack(offset);
                break;
            }
        }
    }
}

void SymbolTable::setSymbolDataLocation(const std::string& name, size_t offset) {
    Symbol symbol("", SymbolKind::LOCAL_VAR, VarType::UNKNOWN, 0);
    if (lookup(name, symbol)) {
        // Find the scope level where this symbol exists
        for (int level = current_scope_level_; level >= 0; level--) {
            auto& scope = scope_stack_[level];
            auto it = scope.find(name);
            if (it != scope.end()) {
                it->second.location = SymbolLocation::data(offset);
                break;
            }
        }
    }
}

void SymbolTable::setSymbolAbsoluteValue(const std::string& name, int64_t value) {
    Symbol symbol("", SymbolKind::LOCAL_VAR, VarType::UNKNOWN, 0);
    if (lookup(name, symbol)) {
        // Find the scope level where this symbol exists
        for (int level = current_scope_level_; level >= 0; level--) {
            auto& scope = scope_stack_[level];
            auto it = scope.find(name);
            if (it != scope.end()) {
                it->second.location = SymbolLocation::absolute(value);
                break;
            }
        }
    }
}

void SymbolTable::addParameter(const std::string& function_name, 
                              const std::string& param_name, 
                              VarType type) {
    // Add to function parameters list
    function_parameters_[function_name].push_back(param_name);
    
    // Add parameter as a symbol
    Symbol param_symbol(param_name, SymbolKind::PARAMETER, type, current_scope_level_);
    addSymbol(param_symbol);
}

std::vector<std::string> SymbolTable::getFunctionParameters(const std::string& function_name) const {
    auto it = function_parameters_.find(function_name);
    if (it != function_parameters_.end()) {
        return it->second;
    }
    return {};
}

void SymbolTable::setCurrentFunction(const std::string& function_name) {
    current_function_ = function_name;
}

std::vector<Symbol> SymbolTable::getAllSymbols() const {
    std::vector<Symbol> all_symbols;
    
    for (const auto& scope : scope_stack_) {
        for (const auto& [name, symbol] : scope) {
            all_symbols.push_back(symbol);
        }
    }
    
    return all_symbols;
}

std::vector<Symbol> SymbolTable::getSymbolsInScope(int scope_level) const {
    std::vector<Symbol> symbols;
    
    if (scope_level < 0 || scope_level >= static_cast<int>(scope_stack_.size())) {
        return symbols; // Invalid scope level
    }
    
    const auto& scope = scope_stack_[scope_level];
    for (const auto& [name, symbol] : scope) {
        symbols.push_back(symbol);
    }
    
    return symbols;
}

std::vector<Symbol> SymbolTable::getGlobalSymbols() const {
    return getSymbolsInScope(0);
}

std::vector<Symbol> SymbolTable::getLocalSymbols() const {
    std::vector<Symbol> locals;
    
    for (int level = 1; level <= current_scope_level_; level++) {
        const auto& scope = scope_stack_[level];
        for (const auto& [name, symbol] : scope) {
            locals.push_back(symbol);
        }
    }
    
    return locals;
}

void SymbolTable::dumpTable() const {
    std::cout << toString() << std::endl;
}

std::string SymbolTable::toString() const {
    std::ostringstream oss;
    
    oss << "Symbol Table (Current Scope Level: " << current_scope_level_ << ")\n";
    oss << "==================================================\n";
    
    for (size_t level = 0; level < scope_stack_.size(); level++) {
        const auto& scope = scope_stack_[level];
        oss << "Scope Level " << level << ":\n";
        
        for (const auto& [name, symbol] : scope) {
            oss << "  " << symbol.to_string() << "\n";
        }
        
        if (scope.empty()) {
            oss << "  <empty>\n";
        }
        
        oss << "\n";
    }
    
    if (!current_function_.empty()) {
        oss << "Current Function: " << current_function_ << "\n";
        
        auto params = getFunctionParameters(current_function_);
        if (!params.empty()) {
            oss << "Parameters: ";
            for (size_t i = 0; i < params.size(); i++) {
                if (i > 0) oss << ", ";
                oss << params[i];
            }
            oss << "\n";
        }
    }
    
    return oss.str();
}

