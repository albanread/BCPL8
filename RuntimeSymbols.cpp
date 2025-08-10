#include "RuntimeSymbols.h"
#include <iostream>
#include <optional>



void RuntimeSymbols::registerAll(SymbolTable& symbol_table) {
    // Standard runtime integer-returning functions
    registerRuntimeFunction(symbol_table, "READN");
    registerRuntimeFunction(symbol_table, "RND", {
        {VarType::INTEGER, false} // seed parameter
    });
    registerRuntimeFunction(symbol_table, "LENGTH", {
        {VarType::INTEGER, false} // string pointer
    });
    registerRuntimeFunction(symbol_table, "GETBYTE", {
        {VarType::INTEGER, false}, // pointer
        {VarType::INTEGER, false}  // offset
    });
    registerRuntimeFunction(symbol_table, "GETWORD", {
        {VarType::INTEGER, false}, // pointer
        {VarType::INTEGER, false}  // offset
    });
    
    // Floating-point runtime functions
    registerRuntimeFloatFunction(symbol_table, "READF");
    registerRuntimeFloatFunction(symbol_table, "FLTOFX", {
        {VarType::INTEGER, false}  // integer to convert
    });
    registerRuntimeFloatFunction(symbol_table, "FSIN", {
        {VarType::FLOAT, false}    // angle in radians
    });
    registerRuntimeFloatFunction(symbol_table, "FCOS", {
        {VarType::FLOAT, false}    // angle in radians
    });
    registerRuntimeFloatFunction(symbol_table, "FSQRT", {
        {VarType::FLOAT, false}    // value
    });
    registerRuntimeFloatFunction(symbol_table, "FTAN", {
        {VarType::FLOAT, false}    // angle in radians
    });
    registerRuntimeFloatFunction(symbol_table, "FABS", {
        {VarType::FLOAT, false}    // value
    });
    registerRuntimeFloatFunction(symbol_table, "FLOG", {
        {VarType::FLOAT, false}    // value
    });
    registerRuntimeFloatFunction(symbol_table, "FEXP", {
        {VarType::FLOAT, false}    // value
    });
    
    // Void-returning runtime routines
    registerRuntimeRoutine(symbol_table, "WRITES", {
        {VarType::INTEGER, false}  // string pointer
    });
    registerRuntimeRoutine(symbol_table, "WRITEN", {
        {VarType::INTEGER, false}  // integer value
    });
    registerRuntimeFloatRoutine(symbol_table, "WRITEF", {
        {VarType::FLOAT, false}    // float value
    });
    registerRuntimeRoutine(symbol_table, "PUTBYTE", {
        {VarType::INTEGER, false}, // pointer
        {VarType::INTEGER, false}, // offset
        {VarType::INTEGER, false}  // value
    });
    registerRuntimeRoutine(symbol_table, "PUTWORD", {
        {VarType::INTEGER, false}, // pointer
        {VarType::INTEGER, false}, // offset
        {VarType::INTEGER, false}  // value
    });
    registerRuntimeRoutine(symbol_table, "EXIT", {
        {VarType::INTEGER, false}  // exit code
    });
    registerRuntimeRoutine(symbol_table, "NEWLINE");
    registerRuntimeRoutine(symbol_table, "NEWPAGE");
}

void RuntimeSymbols::registerRuntimeFunction(
    SymbolTable& symbol_table, 
    const std::string& name,
    const std::vector<Symbol::ParameterInfo>& params
) {
    // Create a new symbol for this runtime function
    Symbol symbol(name, SymbolKind::RUNTIME_FUNCTION, VarType::INTEGER, 0);
    
    // Add parameter information
    symbol.parameters = params;
    
    // Register in the global scope
    if (!symbol_table.addSymbol(symbol)) {
        std::cerr << "Warning: Could not register runtime function " << name 
                  << " (duplicate symbol)" << std::endl;
    }
}

void RuntimeSymbols::registerRuntimeFloatFunction(
    SymbolTable& symbol_table, 
    const std::string& name,
    const std::vector<Symbol::ParameterInfo>& params
) {
    // Create a new symbol for this float runtime function
    Symbol symbol(name, SymbolKind::RUNTIME_FLOAT_FUNCTION, VarType::FLOAT, 0);
    
    // Add parameter information
    symbol.parameters = params;
    
    // Register in the global scope
    if (!symbol_table.addSymbol(symbol)) {
        std::cerr << "Warning: Could not register runtime float function " << name 
                  << " (duplicate symbol)" << std::endl;
    }
}

void RuntimeSymbols::registerRuntimeRoutine(
    SymbolTable& symbol_table, 
    const std::string& name,
    const std::vector<Symbol::ParameterInfo>& params
) {
    // Create a new symbol for this runtime routine
    Symbol symbol(name, SymbolKind::RUNTIME_ROUTINE, VarType::INTEGER, 0);
    
    // Add parameter information
    symbol.parameters = params;
    
    // Register in the global scope
    if (!symbol_table.addSymbol(symbol)) {
        std::cerr << "Warning: Could not register runtime routine " << name 
                  << " (duplicate symbol)" << std::endl;
    }
}

void RuntimeSymbols::registerRuntimeFloatRoutine(
    SymbolTable& symbol_table, 
    const std::string& name,
    const std::vector<Symbol::ParameterInfo>& params
) {
    // Create a new symbol for this float-handling runtime routine
    Symbol symbol(name, SymbolKind::RUNTIME_FLOAT_ROUTINE, VarType::FLOAT, 0);
    
    // Add parameter information
    symbol.parameters = params;
    
    // Register in the global scope
    if (!symbol_table.addSymbol(symbol)) {
        std::cerr << "Warning: Could not register runtime float routine " << name 
                  << " (duplicate symbol)" << std::endl;
    }
}

