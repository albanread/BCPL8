#include "../ASTAnalyzer.h"
#include "../../DataTypes.h"
#include "../../RuntimeManager.h"
#include "../../SymbolTable.h"
#include "../../Symbol.h"
#include <iostream>
#include <string>
#include <map>
#include "../../AST.h"

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

void ASTAnalyzer::visit(ListExpression& node) {
    for (auto& expr : node.initializers) {
        if (expr) expr->accept(*this);
    }
}

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

void ASTAnalyzer::visit(VecInitializerExpression& node) {
    // No-op: type inference is handled in infer_expression_type.
}

void ASTAnalyzer::visit(ForEachStatement& node) {
    if (node.collection_expression) node.collection_expression->accept(*this);
    if (node.body) node.body->accept(*this);

    VarType collection_type = infer_expression_type(node.collection_expression.get());

    // --- Centralized element type inference for all collection types ---
    VarType element_type = VarType::ANY; // Default for ANY_LIST

    switch(collection_type) {
        // Vectors and Strings
        case VarType::POINTER_TO_INT_VEC:
            element_type = VarType::INTEGER;
            break;
        case VarType::POINTER_TO_FLOAT_VEC:
            element_type = VarType::FLOAT;
            break;
        case VarType::POINTER_TO_STRING:
            element_type = VarType::INTEGER; // Characters are integers
            break;

        // All List Types
        case VarType::POINTER_TO_INT_LIST:
        case VarType::CONST_POINTER_TO_INT_LIST:
            element_type = VarType::INTEGER;
            break;
        case VarType::POINTER_TO_FLOAT_LIST:
        case VarType::CONST_POINTER_TO_FLOAT_LIST:
            element_type = VarType::FLOAT;
            break;
        case VarType::POINTER_TO_STRING_LIST:
        case VarType::CONST_POINTER_TO_STRING_LIST:
            element_type = VarType::POINTER_TO_STRING;
            break;

        // Default for ANY_LIST
        default:
            // For ANY_LIST, element_type remains ANY unless a filter is applied later.
            break;
    }

    // Register loop variables in the current function's scope with correct types
    if (!current_function_scope_.empty()) {
        auto& metrics = function_metrics_[current_function_scope_];

        if (!node.type_variable_name.empty()) { // Two-variable form: FOREACH T, V
            metrics.variable_types[node.type_variable_name] = VarType::INTEGER;

            bool is_list = (collection_type >= VarType::POINTER_TO_ANY_LIST && collection_type <= VarType::CONST_POINTER_TO_STRING_LIST);

            if (is_list) {
                metrics.variable_types[node.loop_variable_name] = VarType::ANY; // V is the node pointer
                node.filter_type = VarType::ANY;
            } else {
                metrics.variable_types[node.loop_variable_name] = element_type; // V is the element value
            }
        } else { // One-variable form
            if (collection_type == VarType::POINTER_TO_ANY_LIST || collection_type == VarType::CONST_POINTER_TO_ANY_LIST) {
                element_type = node.filter_type; // Refine type for ANY_LIST if a filter exists
            }
            metrics.variable_types[node.loop_variable_name] = element_type;

            // --- FIX IS HERE ---
            // Also update the symbol table with the correct inferred type.
            if (symbol_table_) {
                symbol_table_->updateSymbolType(node.loop_variable_name, element_type);
            }
            // --- END OF FIX ---
        }
    }

    // Store the final inferred element type in the node for the CFG builder.
    node.inferred_element_type = element_type;
}

// --- BitfieldAccessExpression visitor ---
void ASTAnalyzer::visit(BitfieldAccessExpression& node) {
    if (node.base_expr) node.base_expr->accept(*this);
    if (node.start_bit_expr) node.start_bit_expr->accept(*this);
    if (node.width_expr) node.width_expr->accept(*this);
}



ASTAnalyzer& ASTAnalyzer::getInstance() {
    static ASTAnalyzer instance;
    return instance;
}

ASTAnalyzer::ASTAnalyzer() {
    reset_state();
}

// --- Type inference for expressions ---
VarType ASTAnalyzer::infer_expression_type(const Expression* expr) const {
    if (!expr) return VarType::INTEGER;

    if (auto* lit = dynamic_cast<const NumberLiteral*>(expr)) {
        return lit->literal_type == NumberLiteral::LiteralType::Float ? VarType::FLOAT : VarType::INTEGER;
    }
    if (dynamic_cast<const StringLiteral*>(expr)) {
        return VarType::POINTER_TO_STRING;
    }
    if (auto* var = dynamic_cast<const VariableAccess*>(expr)) {
        // Use the current function scope to look up the variable type
        return get_variable_type(current_function_scope_, var->name);
    }
    if (auto* call = dynamic_cast<const FunctionCall*>(expr)) {
        if (auto* func_var = dynamic_cast<const VariableAccess*>(call->function_expr.get())) {
            // List of modifying functions
            static const std::set<std::string> modifying_funcs = {
                "REVERSE", "APND", "FILTER"
            };

            // Special handling for CONCAT to preserve specific list type if both args match
            if (func_var->name == "CONCAT") {
                if (call->arguments.size() == 2) {
                    VarType list1_type = infer_expression_type(call->arguments[0].get());
                    VarType list2_type = infer_expression_type(call->arguments[1].get());
                    if (list1_type == list2_type) {
                        return list1_type;
                    }
                }
                return VarType::POINTER_TO_ANY_LIST;
            }

            if (modifying_funcs.count(func_var->name)) {
                if (!call->arguments.empty()) {
                    VarType list_arg_type = infer_expression_type(call->arguments[0].get());
                    if (is_const_list_type(list_arg_type)) {
                        std::cerr << "Semantic Error: Cannot use modifying function '"
                                  << func_var->name
                                  << "' on a read-only MANIFESTLIST." << std::endl;
                        // Optionally: set an error flag or throw
                    }
                }
                return VarType::POINTER_TO_ANY_LIST;
            }
            // Recognize LIST, COPYLIST, DEEPCOPYLIST, FIND as returning a list pointer
            if (func_var->name == "LIST" || func_var->name == "COPYLIST" || func_var->name == "DEEPCOPYLIST" ||
                func_var->name == "FIND") {
                return VarType::POINTER_TO_ANY_LIST;
            }
            // --- SPLIT and JOIN built-in string/list functions ---
            if (func_var->name == "SPLIT") {
                // SPLIT returns a list of strings, which is a pointer to a list of strings.
                return VarType::POINTER_TO_STRING_LIST;
            }
            if (func_var->name == "JOIN") {
                // JOIN returns a new string, which is a pointer to a string.
                return VarType::POINTER_TO_STRING;
            }
            // --- AS_* built-in type-casting intrinsics ---
            if (func_var->name == "AS_INT") {
                return VarType::INTEGER;
            }
            if (func_var->name == "AS_FLOAT") {
                return VarType::FLOAT;
            }
            if (func_var->name == "AS_STRING") {
                return VarType::POINTER_TO_STRING;
            }
            if (func_var->name == "AS_LIST") {
                return VarType::POINTER_TO_ANY_LIST;
            }
            // --- end AS_* built-ins ---
            auto it = function_return_types_.find(func_var->name);
            if (it != function_return_types_.end()) {
                return it->second;
            }
        }
    }
    if (auto* bin_op = dynamic_cast<const BinaryOp*>(expr)) {
        // If either operand is a float, the result is a float.
        if (infer_expression_type(bin_op->left.get()) == VarType::FLOAT ||
            infer_expression_type(bin_op->right.get()) == VarType::FLOAT) {
            return VarType::FLOAT;
        }
    }
    if (auto* un_op = dynamic_cast<const UnaryOp*>(expr)) {
        if (un_op->op == UnaryOp::Operator::FloatConvert) {
            return VarType::FLOAT;
        }
        if (un_op->op == UnaryOp::Operator::AddressOf) {
            VarType base_type = infer_expression_type(un_op->operand.get());
            if (base_type == VarType::FLOAT) return VarType::POINTER_TO_FLOAT;
            if (base_type == VarType::INTEGER) return VarType::POINTER_TO_INT;
        }
        if (un_op->op == UnaryOp::Operator::Indirection) {
            VarType ptr_type = infer_expression_type(un_op->operand.get());
            if (ptr_type == VarType::POINTER_TO_FLOAT) return VarType::FLOAT;
            if (ptr_type == VarType::POINTER_TO_INT) return VarType::INTEGER;
            if (ptr_type == VarType::POINTER_TO_FLOAT_VEC) return VarType::FLOAT;
        }
        // --- REST (TailOfNonDestructive) ---
        if (un_op->op == UnaryOp::Operator::TailOfNonDestructive) {
            // REST returns a pointer to a list node (the tail)
            return VarType::POINTER_TO_LIST_NODE;
        }
        if (un_op->op == UnaryOp::Operator::HeadOf) {
            // HD returns the element type of the list
            VarType operand_type = infer_expression_type(un_op->operand.get());
            // Handle all list types
            if (operand_type == VarType::POINTER_TO_INT_LIST || operand_type == VarType::POINTER_TO_LIST_NODE) {
                return VarType::INTEGER;
            }
            if (operand_type == VarType::POINTER_TO_FLOAT_LIST) {
                return VarType::FLOAT;
            }
            if (operand_type == VarType::POINTER_TO_ANY_LIST) {
                return VarType::ANY;
            }
            return VarType::UNKNOWN;
        }
        if (un_op->op == UnaryOp::Operator::TailOf) {
            // TL returns the same list type as its operand
            VarType operand_type = infer_expression_type(un_op->operand.get());
            if (operand_type == VarType::POINTER_TO_INT_LIST ||
                operand_type == VarType::POINTER_TO_FLOAT_LIST ||
                operand_type == VarType::POINTER_TO_ANY_LIST) {
                return operand_type;
            }
            return VarType::UNKNOWN;
        }
        // --- LEN intrinsic type checking ---
        if (un_op->op == UnaryOp::Operator::LengthOf) {
            VarType operand_type = infer_expression_type(un_op->operand.get());
            if (
                operand_type == VarType::POINTER_TO_INT_VEC ||      // For VEC
                operand_type == VarType::POINTER_TO_FLOAT_VEC ||    // For FVEC
                operand_type == VarType::POINTER_TO_STRING ||       // For STRING
                operand_type == VarType::POINTER_TO_TABLE ||        // For TABLE
                operand_type == VarType::POINTER_TO_INT_LIST ||
                operand_type == VarType::POINTER_TO_FLOAT_LIST ||
                operand_type == VarType::POINTER_TO_ANY_LIST
            ) {
                return VarType::INTEGER;
            } else {
                std::cerr << "Semantic Error: LEN operator applied to an invalid type." << std::endl;
                return VarType::UNKNOWN;
            }
        }
    }
    // BitfieldAccessExpression always yields an integer
    if (auto* bf = dynamic_cast<const BitfieldAccessExpression*>(expr)) {
        return VarType::INTEGER;
    }
    // Add logic for vector allocations
    if (auto* vec = dynamic_cast<const VecAllocationExpression*>(expr)) {
        return VarType::POINTER_TO_INT_VEC;
    }
    if (auto* fvec = dynamic_cast<const FVecAllocationExpression*>(expr)) {
        return VarType::POINTER_TO_FLOAT_VEC;
    }
    if (auto* table = dynamic_cast<const TableExpression*>(expr)) {
        if (table->is_float_table) {
            return VarType::POINTER_TO_FLOAT_VEC;
        } else {
            return VarType::POINTER_TO_INT_VEC;
        }
    }
    if (auto* str_alloc = dynamic_cast<const StringAllocationExpression*>(expr)) {
        return VarType::POINTER_TO_STRING;
    }
    if (auto* list_expr = dynamic_cast<const ListExpression*>(expr)) {
        // If the list is empty, we can't infer an element type.
        if (list_expr->initializers.empty()) {
            // Manifest lists are still const, even if empty
            if (list_expr->is_manifest) {
                return VarType::CONST_POINTER_TO_ANY_LIST;
            }
            return VarType::POINTER_TO_ANY_LIST;
        }

        // Infer the type of the first element to establish a baseline.
        VarType first_element_type = infer_expression_type(list_expr->initializers[0].get());
        bool all_match = true;

        // Check all subsequent elements against the first one.
        for (size_t i = 1; i < list_expr->initializers.size(); ++i) {
            if (infer_expression_type(list_expr->initializers[i].get()) != first_element_type) {
                all_match = false;
                break;
            }
        }

        // If all elements have the same type, return the specific list type.
        if (list_expr->is_manifest) {
            // MANIFESTLIST: Return the const version of the type
            if (all_match && first_element_type == VarType::FLOAT) {
                return VarType::CONST_POINTER_TO_FLOAT_LIST;
            }
            if (all_match && first_element_type == VarType::INTEGER) {
                return VarType::CONST_POINTER_TO_INT_LIST;
            }
            if (all_match && first_element_type == VarType::POINTER_TO_STRING) {
                return VarType::CONST_POINTER_TO_STRING_LIST;
            }
            return VarType::CONST_POINTER_TO_ANY_LIST;
        } else {
            // Regular LIST: Return the mutable version
            if (all_match && first_element_type == VarType::FLOAT) {
                return VarType::POINTER_TO_FLOAT_LIST;
            }
            if (all_match && first_element_type == VarType::INTEGER) {
                return VarType::POINTER_TO_INT_LIST;
            }
            if (all_match && first_element_type == VarType::POINTER_TO_STRING) {
                return VarType::POINTER_TO_STRING_LIST;
            }
            return VarType::POINTER_TO_ANY_LIST;
        }
    }
    // Add logic for VEC initializer lists
    if (auto* vec_init = dynamic_cast<const VecInitializerExpression*>(expr)) {
        // If the list is empty, default to an empty integer vector.
        if (vec_init->initializers.empty()) {
            return VarType::POINTER_TO_INT_VEC;
        }

        // Infer the type of the first element to establish a baseline.
        VarType first_element_type = infer_expression_type(vec_init->initializers[0].get());

        // Check all subsequent elements to ensure they are the same type.
        for (size_t i = 1; i < vec_init->initializers.size(); ++i) {
            if (infer_expression_type(vec_init->initializers[i].get()) != first_element_type) {
                // Heterogeneous vectors are not supported. This is a semantic error.
                std::cerr << "Semantic Error: VEC initializers must all be of the same type (either all integer or all float)." << std::endl;
                return VarType::UNKNOWN; // Return an error type
            }
        }

        // If all elements have the same type, return the specific vector type.
        if (first_element_type == VarType::FLOAT) {
            return VarType::POINTER_TO_FLOAT_VEC;
        } else {
            // For integers, pointers, etc., use a standard integer vector.
            return VarType::POINTER_TO_INT_VEC;
        }
    }
    return VarType::UNKNOWN;
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
    // --- Bitwise OR support for type constants ---
    if (auto* bin_op = dynamic_cast<BinaryOp*>(expr)) {
        bool left_has_value, right_has_value;
        int64_t left_val = evaluate_constant_expression(bin_op->left.get(), &left_has_value);
        int64_t right_val = evaluate_constant_expression(bin_op->right.get(), &right_has_value);
        if (left_has_value && right_has_value) {
            if (bin_op->op == BinaryOp::Operator::BitwiseOr) { // bitwise OR operator
                *has_value = true;
                return left_val | right_val;
            }
            if (bin_op->op == BinaryOp::Operator::LogicalOr) { // logical OR operator
                *has_value = true;
                return (left_val != 0 || right_val != 0) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0);
            }
            // Optionally, add support for other operators if needed
        }
    }
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