#ifndef RUNTIME_MANAGER_H
#define RUNTIME_MANAGER_H
#pragma once

#include <string>
#include <unordered_map>
#include <stdexcept>
#include "DataTypes.h"

// Structure to hold information about a runtime function
struct RuntimeFunction {
    std::string name;
    int num_args; // Number of arguments the function expects
    // Placeholder for the actual address, to be filled by the JIT/Linker
    // For now, we'll use a size_t, which can hold a pointer.
    void* address; 
    FunctionType type; // Add this line

    // Update the constructor to accept the type.
    RuntimeFunction(std::string n, int args, void* addr, FunctionType t = FunctionType::STANDARD)
        : name(std::move(n)), num_args(args), address(addr), type(t) {}
};

// Manages information about external runtime functions
class RuntimeManager {
public:
    // Singleton accessor
    static RuntimeManager& instance() {
        static RuntimeManager instance;
        return instance;
    }

    // Enables tracing for runtime operations.
    void enableTracing() { trace_enabled_ = true; }

    // Checks if tracing is enabled.
    bool isTracingEnabled() const { return trace_enabled_; }

    // Registers a new runtime function.
    void register_function(const std::string& name, int num_args, void* address, FunctionType type = FunctionType::STANDARD);

    // Retrieves information about a registered runtime function.
    const RuntimeFunction& get_function(const std::string& name) const;

    // Checks if a function is registered.
    bool is_function_registered(const std::string& name) const;

    // Utility: Convert a string to uppercase for case-insensitive lookup
    static std::string to_upper(const std::string& s);

    

    // Sets the actual address of a runtime function (used by JIT/Linker).
    void set_function_address(const std::string& name, void* address);

    // Gets the map of all registered functions.
    const std::unordered_map<std::string, RuntimeFunction>& get_registered_functions() const { return functions_; }

    // Prints all registered runtime functions, their addresses, and argument counts
    void print_registered_functions() const;

private:
    // Flag to indicate if tracing is enabled.
    bool trace_enabled_ = false;

private:
    RuntimeManager();
    RuntimeManager(const RuntimeManager&) = delete;
    RuntimeManager& operator=(const RuntimeManager&) = delete;
    RuntimeManager(RuntimeManager&&) = delete;
    RuntimeManager& operator=(RuntimeManager&&) = delete;

    std::unordered_map<std::string, RuntimeFunction> functions_;
};

#endif // RUNTIME_MANAGER_H
