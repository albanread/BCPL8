#ifndef SYMBOL_H
#define SYMBOL_H

#include <string>
#include <variant>
#include <memory>
#include <vector>
#include "DataTypes.h"  // Include for VarType and FunctionType definitions
#include <optional>



// The kinds of symbols that can exist in BCPL
enum class SymbolKind {
    LOCAL_VAR,            // Local variable
    STATIC_VAR,           // Static variable 
    GLOBAL_VAR,           // Global variable
    PARAMETER,            // Function parameter
    FUNCTION,             // Function (returns an integer value)
    FLOAT_FUNCTION,       // Function (returns a float value)
    ROUTINE,              // Routine (doesn't return a value)
    LABEL,                // Code label
    MANIFEST,             // Manifest constant
    RUNTIME_FUNCTION,     // Runtime function (returns an integer value)
    RUNTIME_FLOAT_FUNCTION, // Runtime function (returns a float value)
    RUNTIME_ROUTINE,      // Runtime routine (doesn't return a value)
    RUNTIME_FLOAT_ROUTINE // Runtime routine (handles float parameters)
};

// Location information for a symbol
struct SymbolLocation {
    enum class LocationType {
        STACK,      // Local variables and parameters (stack-allocated)
        DATA,       // Static/global variables (data segment)
        ABSOLUTE,   // Manifest constants (fixed value)
        LABEL,      // Code address
        UNKNOWN     // Not yet allocated
    };
    
    LocationType type = LocationType::UNKNOWN;
    
    // Union of possible location values based on type
    union {
        int stack_offset;        // For STACK: offset from frame pointer
        size_t data_offset;      // For DATA: offset in the data segment
        int64_t absolute_value;  // For ABSOLUTE: manifest constant value
        // For LABEL: the label name is stored in the Symbol's name field
    };
    
    // Default constructor initializes to UNKNOWN location
    SymbolLocation() : type(LocationType::UNKNOWN) {}
    
    // Stack location constructor
    static SymbolLocation stack(int offset) {
        SymbolLocation loc;
        loc.type = LocationType::STACK;
        loc.stack_offset = offset;
        return loc;
    }
    
    // Data segment location constructor
    static SymbolLocation data(size_t offset) {
        SymbolLocation loc;
        loc.type = LocationType::DATA;
        loc.data_offset = offset;
        return loc;
    }
    
    // Absolute value location constructor
    static SymbolLocation absolute(int64_t value) {
        SymbolLocation loc;
        loc.type = LocationType::ABSOLUTE;
        loc.absolute_value = value;
        return loc;
    }
    
    // Label location constructor
    static SymbolLocation label() {
        SymbolLocation loc;
        loc.type = LocationType::LABEL;
        return loc;
    }
};

// Complete symbol information
class Symbol {
public:
    Symbol() : name(""), kind(SymbolKind::LOCAL_VAR), type(VarType::UNKNOWN), scope_level(0) {}
    
    Symbol(const std::string& name, SymbolKind kind, VarType type, int scope_level)
        : name(name), kind(kind), type(type), scope_level(scope_level) {}
    
    // Core symbol attributes
    std::string name;    // Symbol identifier
    SymbolKind kind;     // What kind of symbol this is
    VarType type;        // Data type (INTEGER, FLOAT)
    int scope_level;     // Scope level (0 for global)
    
    // Location information (may be populated later)
    SymbolLocation location;
    
    // For arrays and vectors
    size_t size = 0;
    bool has_size = false;  // Flag to indicate if size is set

    // Parameter information for functions and routines
    struct ParameterInfo {
        VarType type;
        bool is_optional;
    };
    std::vector<ParameterInfo> parameters;
    
        // Helper methods
        bool is_local() const { 
            return kind == SymbolKind::LOCAL_VAR || kind == SymbolKind::PARAMETER; 
        }
    
        bool is_global() const { 
            return kind == SymbolKind::GLOBAL_VAR || kind == SymbolKind::STATIC_VAR; 
        }
    
        bool is_function_like() const {
            return kind == SymbolKind::FUNCTION || kind == SymbolKind::FLOAT_FUNCTION || 
                   kind == SymbolKind::ROUTINE || is_runtime();
        }
    
        bool is_integer_function() const {
            return kind == SymbolKind::FUNCTION || kind == SymbolKind::RUNTIME_FUNCTION;
        }
    
        bool is_float_function() const {
            return kind == SymbolKind::FLOAT_FUNCTION || kind == SymbolKind::RUNTIME_FLOAT_FUNCTION || 
                   kind == SymbolKind::RUNTIME_FLOAT_ROUTINE;
        }
    
        FunctionType get_function_type() const {
            return is_float_function() ? FunctionType::FLOAT : FunctionType::STANDARD;
        }
    
        bool is_variable() const {
            return kind == SymbolKind::LOCAL_VAR || kind == SymbolKind::STATIC_VAR ||
                   kind == SymbolKind::GLOBAL_VAR || kind == SymbolKind::PARAMETER;
        }
    
        bool is_runtime() const {
            return kind == SymbolKind::RUNTIME_FUNCTION || 
                   kind == SymbolKind::RUNTIME_FLOAT_FUNCTION || 
                   kind == SymbolKind::RUNTIME_ROUTINE ||
                   kind == SymbolKind::RUNTIME_FLOAT_ROUTINE;
        }
    
        bool is_runtime_function() const {
            return kind == SymbolKind::RUNTIME_FUNCTION || 
                   kind == SymbolKind::RUNTIME_FLOAT_FUNCTION;
        }
    
        bool is_runtime_routine() const {
            return kind == SymbolKind::RUNTIME_ROUTINE || 
                   kind == SymbolKind::RUNTIME_FLOAT_ROUTINE;
        }
    
    // String representation for debugging
    std::string to_string() const;
};



#endif // SYMBOL_H