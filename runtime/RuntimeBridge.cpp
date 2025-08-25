// RuntimeBridge.cpp
// Implementation of the runtime bridge between new C-compatible runtime and RuntimeManager

#include "RuntimeBridge.h"
#include "runtime.h"
#include "../RuntimeManager.h"
#include <iostream>
#include <string>
#include "heap_interface.h"

// Forward declarations for runtime functions
extern "C" {
    void bcpl_free_list(void*);
    int64_t BCPL_LIST_GET_HEAD_AS_INT(void*);
    double BCPL_LIST_GET_HEAD_AS_FLOAT(void*);
    void* BCPL_LIST_GET_TAIL(void*);
    void* BCPL_LIST_GET_REST(void*);
    int64_t BCPL_GET_ATOM_TYPE(void*);
    void BCPL_LIST_APPEND_LIST(void*, void*);
    void* BCPL_LIST_GET_NTH(void*, int64_t);

    // New list creation/append ABI
    ListHeader* BCPL_LIST_CREATE_EMPTY(void);
    void BCPL_LIST_APPEND_INT(void*, int64_t);
    void BCPL_LIST_APPEND_FLOAT(void*, double);
    void BCPL_LIST_APPEND_STRING(ListHeader* header, uint32_t* value);
    // Add more as needed for other types

    // Free list helpers
    void BCPL_FREE_CELLS(void);
    void* get_g_free_list_head_address(void);
    void returnNodeToFreelist_runtime(void*);
}


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
    register_runtime_function("BCPL_FREE_LIST", 1, reinterpret_cast<void*>(bcpl_free_list));
    register_runtime_function("BCPL_LIST_GET_HEAD_AS_INT", 1, reinterpret_cast<void*>(BCPL_LIST_GET_HEAD_AS_INT));
    register_runtime_function("BCPL_LIST_GET_HEAD_AS_FLOAT", 1, reinterpret_cast<void*>(BCPL_LIST_GET_HEAD_AS_FLOAT), FunctionType::FLOAT);
    register_runtime_function("BCPL_LIST_GET_TAIL", 1, reinterpret_cast<void*>(BCPL_LIST_GET_TAIL));
    register_runtime_function("BCPL_LIST_GET_REST", 1, reinterpret_cast<void*>(BCPL_LIST_GET_REST));
    register_runtime_function("BCPL_GET_ATOM_TYPE", 1, reinterpret_cast<void*>(BCPL_GET_ATOM_TYPE));
    register_runtime_function("BCPL_LIST_GET_NTH", 2, reinterpret_cast<void*>(BCPL_LIST_GET_NTH));

    // --- Register SETTYPE as a pseudo-runtime function ---
    // The address is null because the ASTAnalyzer will handle it and prevent any code from being generated.
    register_runtime_function("SETTYPE", 2, nullptr);

    // New free list helpers
    register_runtime_function("BCPL_FREE_CELLS", 0, reinterpret_cast<void*>(BCPL_FREE_CELLS));
    register_runtime_function("GET_FREE_LIST_HEAD_ADDR", 0, reinterpret_cast<void*>(get_g_free_list_head_address));

    // New list creation/append ABI
    register_runtime_function("BCPL_LIST_CREATE_EMPTY", 0, reinterpret_cast<void*>(BCPL_LIST_CREATE_EMPTY));
    register_runtime_function("BCPL_LIST_APPEND_INT", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_INT));
    register_runtime_function("BCPL_LIST_APPEND_FLOAT", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_FLOAT), FunctionType::FLOAT);
    register_runtime_function("BCPL_LIST_APPEND_STRING", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_STRING));
    // Add more as needed for other types

    // Aliases for BCPL list append for BCPL source-level calls
    register_runtime_function("APND", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_INT));
    register_runtime_function("FPND", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_FLOAT), FunctionType::FLOAT);
    register_runtime_function("SPND", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_STRING));
    // Add this new line for appending lists
    register_runtime_function("LPND", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_LIST));
    register_runtime_function("BCPL_CONCAT_LISTS", 2, reinterpret_cast<void*>(BCPL_CONCAT_LISTS));
    register_runtime_function("CONCAT", 2, reinterpret_cast<void*>(BCPL_CONCAT_LISTS));

    // List copy functions
    register_runtime_function("COPYLIST", 1, reinterpret_cast<void*>(BCPL_SHALLOW_COPY_LIST));
    register_runtime_function("DEEPCOPYLIST", 1, reinterpret_cast<void*>(BCPL_DEEP_COPY_LIST));
    // Register the new function for handling list literals
    register_runtime_function("DEEPCOPYLITERALLIST", 1, reinterpret_cast<void*>(BCPL_DEEP_COPY_LITERAL_LIST));
    register_runtime_function("REVERSE", 1, reinterpret_cast<void*>(BCPL_REVERSE_LIST));
    register_runtime_function("FIND", 3, reinterpret_cast<void*>(BCPL_FIND_IN_LIST));
    register_runtime_function("FILTER", 2, reinterpret_cast<void*>(BCPL_LIST_FILTER));

    // --- Register SPLIT and JOIN string/list functions ---
    register_runtime_function("APND", 2, reinterpret_cast<void*>(BCPL_LIST_APPEND_INT));
    register_runtime_function("JOIN", 2, reinterpret_cast<void*>(BCPL_JOIN_LIST));
    
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
    
    // Register fast freelist return for TL
    register_runtime_function("returnNodeToFreelist", 1, reinterpret_cast<void*>(returnNodeToFreelist_runtime));

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

extern "C" void initialize_freelist();

void initialize_runtime() {
    initialize_freelist(); // Pre-allocate freelist nodes at startup
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
