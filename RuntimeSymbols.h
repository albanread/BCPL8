#ifndef RUNTIME_SYMBOLS_H
#define RUNTIME_SYMBOLS_H

#include "SymbolTable.h"
#include "RuntimeManager.h"
#include "DataTypes.h"
#include <string>
#include <vector>



/**
 * @class RuntimeSymbols
 * @brief Helper class for registering runtime functions in the symbol table
 *
 * This class provides methods to register all runtime functions, routines, and
 * their parameter information in the centralized symbol table.
 */
class RuntimeSymbols {
public:
    /**
     * @brief Register all runtime functions/routines in the symbol table
     * 
     * This method populates the symbol table with all available runtime
     * functions and routines, including their return types and parameter
     * information.
     * 
     * @param symbol_table The symbol table to populate
     */
    static void registerAll(SymbolTable& symbol_table);

private:
    /**
     * @brief Register a runtime function that returns an integer
     * 
     * @param symbol_table The symbol table to update
     * @param name Function name
     * @param params Vector of parameter information
     */
    static void registerRuntimeFunction(
        SymbolTable& symbol_table, 
        const std::string& name,
        const std::vector<Symbol::ParameterInfo>& params = {}
    );

    /**
     * @brief Register a runtime function that returns a float
     * 
     * @param symbol_table The symbol table to update
     * @param name Function name
     * @param params Vector of parameter information
     */
    static void registerRuntimeFloatFunction(
        SymbolTable& symbol_table, 
        const std::string& name,
        const std::vector<Symbol::ParameterInfo>& params = {}
    );

    /**
     * @brief Register a runtime routine (void-returning function)
     * 
     * @param symbol_table The symbol table to update
     * @param name Routine name
     * @param params Vector of parameter information
     */
    static void registerRuntimeRoutine(
        SymbolTable& symbol_table, 
        const std::string& name,
        const std::vector<Symbol::ParameterInfo>& params = {}
    );

    /**
     * @brief Register a runtime routine that handles float parameters
     * 
     * @param symbol_table The symbol table to update
     * @param name Routine name
     * @param params Vector of parameter information
     */
    static void registerRuntimeFloatRoutine(
        SymbolTable& symbol_table, 
        const std::string& name,
        const std::vector<Symbol::ParameterInfo>& params = {}
    );
};



#endif // RUNTIME_SYMBOLS_H