#ifndef RUNTIME_BRIDGE_H
#define RUNTIME_BRIDGE_H

#include "runtime.h"
#include "../RuntimeManager.h"

// This header provides functionality to bridge between the new C-compatible 
// runtime defined in runtime.h and the C++ RuntimeManager


namespace runtime {

/**
 * Registers all standard runtime functions with the RuntimeManager.
 * This should be called during initialization to make runtime functions
 * available for JIT compilation.
 */
void register_runtime_functions();

/**
 * Registers a specific runtime function with the RuntimeManager.
 *
 * @param name      The name of the function (case-insensitive)
 * @param num_args  The number of arguments the function takes
 * @param address   Function pointer to the implementation
 * @param type      The function type (STANDARD, FLOAT, etc.)
 */
void register_runtime_function(
    const std::string& name, 
    int num_args, 
    void* address, 
    FunctionType type = FunctionType::STANDARD);

/**
 * Initializes the runtime system for JIT execution.
 * This should be called once before any JIT-compiled code is executed.
 */
void initialize_runtime();

/**
 * Cleans up any runtime resources.
 * This should be called before the program exits.
 */
void cleanup_runtime();

/**
 * Get version information about the runtime.
 *
 * @return A string containing the runtime version information
 */
std::string get_runtime_version();

} // namespace runtime


#endif // RUNTIME_BRIDGE_H