// RuntimeBridge.cpp
// Implementation of the runtime bridge between new C-compatible runtime and RuntimeManager

#include "RuntimeBridge.h"
#include "runtime.h"
#include "../RuntimeManager.h"
#include <iostream>
#include <string>


namespace runtime {

void register_runtime_functions() {
    auto& manager = RuntimeManager::instance();
    
    // Core I/O functions
    register_runtime_function("WRITES", 1, reinterpret_cast<void*>(WRITES));
    register_runtime_function("WRITEN", 1, reinterpret_cast<void*>(WRITEN));
    register_runtime_function("WRITEF", 1, reinterpret_cast<void*>(WRITEF), FunctionType::FLOAT);
    register_runtime_function("WRITEC", 1, reinterpret_cast<void*>(WRITEC));
    register_runtime_function("RDCH", 0, reinterpret_cast<void*>(RDCH));
    
    // Memory management functions
    register_runtime_function("BCPL_ALLOC_WORDS", 3, reinterpret_cast<void*>(bcpl_alloc_words));
    register_runtime_function("BCPL_ALLOC_CHARS", 1, reinterpret_cast<void*>(bcpl_alloc_chars));
    register_runtime_function("MALLOC", 1, reinterpret_cast<void*>(bcpl_alloc_words)); // Alias for compatibility
    register_runtime_function("FREEVEC", 1, reinterpret_cast<void*>(bcpl_free));
    
    // String functions
    register_runtime_function("STRCOPY", 2, reinterpret_cast<void*>(STRCOPY));
    register_runtime_function("STRCMP", 2, reinterpret_cast<void*>(STRCMP));
    register_runtime_function("STRLEN", 1, reinterpret_cast<void*>(STRLEN));
    register_runtime_function("PACKSTRING", 1, reinterpret_cast<void*>(PACKSTRING));
    register_runtime_function("UNPACKSTRING", 1, reinterpret_cast<void*>(UNPACKSTRING));
    
    // File I/O functions
    register_runtime_function("SLURP", 1, reinterpret_cast<void*>(SLURP));
    register_runtime_function("SPIT", 2, reinterpret_cast<void*>(SPIT));
    
    // System functions
    register_runtime_function("FINISH", 0, reinterpret_cast<void*>(finish));
    
    if (manager.isTracingEnabled()) {
        std::cout << "Registered " << manager.get_registered_functions().size() 
                  << " runtime functions" << std::endl;
        manager.print_registered_functions();
    }
}

void register_runtime_function(
    const std::string& name, 
    int num_args, 
    void* address, 
    FunctionType type) {
    
    try {
        RuntimeManager::instance().register_function(name, num_args, address, type);
    } catch (const std::exception& e) {
        // Function might already be registered - this is often fine during development
        if (RuntimeManager::instance().isTracingEnabled()) {
            std::cerr << "Warning: " << e.what() << std::endl;
        }
    }
}

void initialize_runtime() {
    // Any runtime initialization needed can go here
    if (RuntimeManager::instance().isTracingEnabled()) {
        std::cout << "BCPL Runtime v" << BCPL_RUNTIME_VERSION << " initialized" << std::endl;
    }
}

void cleanup_runtime() {
    // Any runtime cleanup needed can go here
    if (RuntimeManager::instance().isTracingEnabled()) {
        std::cout << "BCPL Runtime shutdown complete" << std::endl;
    }
}

std::string get_runtime_version() {
    return "BCPL Runtime v" + std::string(BCPL_RUNTIME_VERSION);
}

} // namespace runtime
