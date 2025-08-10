#include "../ASTAnalyzer.h"
#include "../../DataTypes.h"
#include "../../RuntimeManager.h"
#include "../../SymbolTable.h"
#include "../../Symbol.h"
#include <iostream>
#include <string>
#include <map>

// This file contains implementations that aren't split into az_*.cpp files
// All duplicated functions have been removed to prevent linker errors

VarType ASTAnalyzer::get_variable_type(const std::string& function_name, const std::string& var_name) const {
    auto it = function_metrics_.find(function_name);
    if (it != function_metrics_.end()) {
        const auto& variable_types = it->second.variable_types;
        auto var_it = variable_types.find(var_name);
        if (var_it != variable_types.end()) {
            return var_it->second;
        }
    }
    return VarType::UNKNOWN;
}

void ASTAnalyzer::visit(FloatValofExpression& node) {
    // Visit the body of the FloatValofExpression if present
    if (node.body) {
        node.body->accept(*this);
    }
    // Additional logic can be added here if needed for FloatValofExpression
}



// --- Stub missing ASTAnalyzer::visit methods ---

void ASTAnalyzer::visit(CharLiteral& node) {}
void ASTAnalyzer::visit(BrkStatement& node) {}
void ASTAnalyzer::visit(GotoStatement& node) {}
void ASTAnalyzer::visit(LoopStatement& node) {}
void ASTAnalyzer::visit(NumberLiteral& node) {}
void ASTAnalyzer::visit(StringLiteral& node) {}
void ASTAnalyzer::visit(BooleanLiteral& node) {}
void ASTAnalyzer::visit(BreakStatement& node) {}
void ASTAnalyzer::visit(FinishStatement& node) {}
void ASTAnalyzer::visit(ReturnStatement& node) {}
void ASTAnalyzer::visit(TableExpression& node) {}
void ASTAnalyzer::visit(EndcaseStatement& node) {}
void ASTAnalyzer::visit(LabelDeclaration& node) {}
void ASTAnalyzer::visit(StaticDeclaration& node) {}



ASTAnalyzer& ASTAnalyzer::getInstance() {
    static ASTAnalyzer instance;
    return instance;
}

ASTAnalyzer::ASTAnalyzer() {
    reset_state();
    symbol_table_ = nullptr;
}

FunctionType ASTAnalyzer::get_runtime_function_type(const std::string& name) const {
    if (RuntimeManager::instance().is_function_registered(name)) {
        return RuntimeManager::instance().get_function(name).type;
    }
    return FunctionType::STANDARD;
}

// Helper to evaluate an expression to a constant integer value.
// Sets has_value to false if the expression is not a compile-time integer constant.
int64_t ASTAnalyzer::evaluate_constant_expression(Expression* expr, bool* has_value) const {
    if (!expr) {
        *has_value = false; // Null expression is not a constant
        return 0;
    }

    // Case 1: Number Literal
    if (auto* number_literal = dynamic_cast<NumberLiteral*>(expr)) {
        if (number_literal->literal_type == NumberLiteral::LiteralType::Integer) {
            *has_value = true;
            return number_literal->int_value;
        }
        // Float literals are not valid integer constants for CASE
        *has_value = false;
        return 0;
    }

    // Case 2: Variable Access
    // After ManifestResolutionPass, any VariableAccess here should not be a manifest.
    // If it is, it's not a compile-time constant for CASE.
    if (dynamic_cast<VariableAccess*>(expr)) {
        *has_value = false;
        return 0;
    }

    // Case 3 (Optional): Simple Constant Folding for Binary/Unary Operations
    // You can extend this to recursively evaluate simple arithmetic operations
    // if all their operands are also compile-time constants.
    // Example for addition:
    // if (auto bin_op = dynamic_cast<BinaryOp*>(expr)) {
    //     bool left_has_value, right_has_value;
    //     int64_t left_val = evaluate_constant_expression(bin_op->left.get(), &left_has_value);
    //     int64_t right_val = evaluate_constant_expression(bin_op->right.get(), &right_has_value);
    //     if (left_has_value && right_has_value) {
    //         *has_value = true;
    //         switch (bin_op->op) {
    //             case BinaryOp::Operator::Add: return left_val + right_val;
    //             case BinaryOp::Operator::Subtract: return left_val - right_val;
    //             // Add other operators as needed (Multiply, Divide, etc.)
    //             default: 
    //                 *has_value = false;
    //                 return 0;
    //         }
    //     }
    // }

    // If it's none of the above (e.g., function call, complex expression, non-integer literal),
    // it's not a compile-time integer constant for a CASE.
    *has_value = false;
    return 0;
}

void ASTAnalyzer::visit(GlobalVariableDeclaration& node) {
    // This is a declaration in the "Global" scope.
    current_function_scope_ = "Global";

    // For each variable in this declaration (e.g., LET G, H = 1, 2)
    for (const auto& name : node.names) {
        // Correctly register this variable's defining scope as "Global".
        variable_definitions_[name] = "Global";
    }
}

bool ASTAnalyzer::is_local_float_function(const std::string& name) const {
    return local_float_function_names_.find(name) != local_float_function_names_.end();
}