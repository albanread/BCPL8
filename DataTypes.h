#ifndef DATA_TYPES_H
#define DATA_TYPES_H

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <optional>

// Forward declare Expression class to avoid circular dependency
class Expression;

// Define ExprPtr using the forward declaration
using ExprPtr = std::unique_ptr<Expression>;

// Define VarType here so it's available to all files that include DataTypes.h
enum class VarType { UNKNOWN, INTEGER, FLOAT, ANY };

enum class FunctionType {
    STANDARD, // For functions using X registers (int/pointer)
    FLOAT     // For functions using D registers (float/double)
};

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

// Structure to hold information about a static variable
struct StaticVariableInfo {
    std::string label;
    ExprPtr initializer;
};

#endif // DATA_TYPES_H
