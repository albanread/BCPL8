#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>
#include <map>

// Forward declare Expression class to avoid circular dependency
class Expression;

// Define ExprPtr using the forward declaration
using ExprPtr = std::unique_ptr<Expression>;

// Define VarType here so it's available to all files that include DataTypes.h
enum class VarType : int64_t {
    UNKNOWN      = 0,
    INTEGER      = 1 << 0, // 1
    FLOAT        = 1 << 1, // 2
    STRING       = 1 << 2, // 4
    ANY          = 1 << 3, // 8

    // Container types
    VEC          = 1 << 8,  // 256
    LIST         = 1 << 9,  // 512
    TABLE        = 1 << 10, // 1024

    // Type Modifiers
    POINTER_TO   = 1 << 12, // 4096
    CONST        = 1 << 13, // 8192

    // --- Examples of Combined Types (for convenience, optional) ---
    POINTER_TO_INT_LIST      = POINTER_TO | LIST | INTEGER,      // 4096 | 512 | 1 = 4609
    POINTER_TO_FLOAT_LIST    = POINTER_TO | LIST | FLOAT,        // 4096 | 512 | 2 = 4610
    POINTER_TO_STRING_LIST   = POINTER_TO | LIST | STRING,       // 4096 | 512 | 4 = 4612
    POINTER_TO_ANY_LIST      = POINTER_TO | LIST | ANY,          // 4096 | 512 | 8 = 4616
    CONST_POINTER_TO_INT_LIST    = CONST | POINTER_TO | LIST | INTEGER,   // 8192 | 4096 | 512 | 1 = 12801
    CONST_POINTER_TO_FLOAT_LIST  = CONST | POINTER_TO | LIST | FLOAT,     // 8192 | 4096 | 512 | 2 = 12802
    CONST_POINTER_TO_STRING_LIST = CONST | POINTER_TO | LIST | STRING,    // 8192 | 4096 | 512 | 4 = 12804
    CONST_POINTER_TO_ANY_LIST    = CONST | POINTER_TO | LIST | ANY,       // 8192 | 4096 | 512 | 8 = 12808

    // --- Legacy/compatibility composite types ---
    POINTER_TO_INT_VEC      = POINTER_TO | VEC | INTEGER,
    POINTER_TO_FLOAT_VEC    = POINTER_TO | VEC | FLOAT,
    POINTER_TO_STRING       = POINTER_TO | STRING,
    POINTER_TO_TABLE        = POINTER_TO | TABLE,
    POINTER_TO_FLOAT        = POINTER_TO | FLOAT,
    POINTER_TO_INT          = POINTER_TO | INTEGER,
    POINTER_TO_LIST_NODE    = POINTER_TO | LIST
};

// Helper function to check for const list types
inline bool is_const_list_type(VarType t) {
    return (static_cast<int64_t>(t) & static_cast<int64_t>(VarType::CONST)) &&
           (static_cast<int64_t>(t) & static_cast<int64_t>(VarType::LIST));
}

enum class FunctionType {
    STANDARD, // For functions using X registers (int/pointer)
    FLOAT     // For functions using D registers (float/double)
};

// Utility to display VarType as a string based on its bitfield flags
inline std::string vartype_to_string(VarType t) {
    int64_t v = static_cast<int64_t>(t);
    if (v == 0) return "UNKNOWN";

    std::string result;
    if (v & static_cast<int64_t>(VarType::CONST)) result += "CONST|";
    if (v & static_cast<int64_t>(VarType::POINTER_TO)) result += "POINTER_TO|";
    if (v & static_cast<int64_t>(VarType::LIST)) result += "LIST|";
    if (v & static_cast<int64_t>(VarType::VEC)) result += "VEC|";
    if (v & static_cast<int64_t>(VarType::TABLE)) result += "TABLE|";
    if (v & static_cast<int64_t>(VarType::INTEGER)) result += "INTEGER|";
    if (v & static_cast<int64_t>(VarType::FLOAT)) result += "FLOAT|";
    if (v & static_cast<int64_t>(VarType::STRING)) result += "STRING|";
    if (v & static_cast<int64_t>(VarType::ANY)) result += "ANY|";

    // Remove trailing '|'
    if (!result.empty() && result.back() == '|') result.pop_back();
    return result;
}

// Structure to hold information about a string literal
struct StringLiteralInfo {
    std::string label;
    std::u32string value;
};

// Structure to hold information about a float literal
struct FloatLiteralInfo {
    std::string label;
    double value;
};

// Structure to hold function metrics, including parameter types for granular inference
struct FunctionMetrics {
    // ... existing members ...
    int num_parameters = 0;
    int num_variables = 0;      // INTEGER variables
    int num_runtime_calls = 0;
    int num_local_function_calls = 0;
    int num_local_routine_calls = 0;
    bool has_vector_allocations = false;
    bool accesses_globals = false;
    std::map<std::string, int> parameter_indices;
    int max_live_variables = 0; // Track peak register pressure for this function

    // START of new members
    int num_float_parameters = 0; // For future use
    int num_float_variables = 0;
    std::map<std::string, VarType> variable_types;
    // END of new members

    std::map<std::string, VarType> parameter_types; // Maps parameter name to its type
};

// Structure to hold information about a static variable
struct StaticVariableInfo {
    std::string label;
    ExprPtr initializer;
};

#endif // DATA_TYPES_H
