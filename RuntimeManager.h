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
    void* address; 
    FunctionType type;
    size_t table_offset; // <-- ADD THIS LINE

    RuntimeFunction(std::string n, int args, void* addr, FunctionType t = FunctionType::STANDARD)
        : name(std::move(n)), num_args(args), address(addr), type(t), table_offset(0) {} // <-- INITIALIZE IN CONSTRUCTOR
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

    // Gets the offset of a runtime function in the global pointer table.
    size_t get_function_offset(const std::string& name) const;

    // Utility: Convert a string to uppercase for case-insensitive lookup
    static std::string to_upper(const std::string& s);

    

    // Sets the actual address of a runtime function (used by JIT/Linker).
    void set_function_address(const std::string& name, void* address);

    // Gets the map of all registered functions.
    const std::unordered_map<std::string, RuntimeFunction>& get_registered_functions() const { return functions_; }

    // Prints all registered runtime functions, their addresses, and argument counts
    void print_registered_functions() const;

    // Populates the X28-relative function pointer table in the .data segment
    void populate_function_pointer_table(void* data_segment_base) const;

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
    size_t next_table_offset_ = 0; // <-- ADD THIS LINE
};

#endif // RUNTIME_MANAGER_H
