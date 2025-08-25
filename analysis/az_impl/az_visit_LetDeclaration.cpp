//
// File: az_visit_LetDeclaration.cpp
// Description: This file implements the `visit` method for `LetDeclaration` nodes
// in the ASTAnalyzer. Its primary role is to perform semantic analysis for `let`
// variable declarations. This includes type inference for the declared variables,
// updating the symbol table with their names and types, and gathering metrics
// about variable counts within function scopes.
//

#include "../ASTAnalyzer.h"
#include "../../SymbolTable.h"

void ASTAnalyzer::visit(LetDeclaration& node) {
    for (size_t i = 0; i < node.names.size(); ++i) {
        const std::string& name = node.names[i];
        Expression* initializer = (i < node.initializers.size()) ? node.initializers[i].get() : nullptr;

        // Semantic analysis and type checking are only performed for local function scopes.
        // Global declarations are handled by a different mechanism.
        if (current_function_scope_ == "Global") {
            if (initializer) {
                initializer->accept(*this);
            }
            continue;
        }

        // 1. Determine the variable's final type based on its initializer and declaration hints.
        VarType determined_type;

        if (initializer) {
            // First, check for explicit vector allocations which have unambiguous types.
            if (dynamic_cast<FVecAllocationExpression*>(initializer)) {
                determined_type = VarType::POINTER_TO_FLOAT_VEC;
            } else if (dynamic_cast<VecAllocationExpression*>(initializer)) {
                determined_type = VarType::POINTER_TO_INT_VEC;
            } else {
                // For all other expressions, use the primary type inferencer.
                VarType inferred_type = infer_expression_type(initializer);

                // If the inferencer returns a complex type (e.g., a list), use it directly.
                if (inferred_type != VarType::UNKNOWN && inferred_type != VarType::INTEGER && inferred_type != VarType::FLOAT) {
                    determined_type = inferred_type;
                } else {
                    // For simple types, combine declaration hints (e.g., `let float x`)
                    // with analysis of the initializer expression.
                    bool is_float = node.is_float_declaration;
                    if (!is_float) {
                        if (auto* num_lit = dynamic_cast<NumberLiteral*>(initializer)) {
                            if (num_lit->literal_type == NumberLiteral::LiteralType::Float) {
                                is_float = true;
                            }
                        } else if (auto* func_call = dynamic_cast<FunctionCall*>(initializer)) {
                            if (auto* var_access = dynamic_cast<VariableAccess*>(func_call->function_expr.get())) {
                                const std::string& func_name = var_access->name;
                                if (is_local_float_function(func_name) || (get_runtime_function_type(func_name) == FunctionType::FLOAT)) {
                                    is_float = true;
                                }
                            }
                        } else if (auto* bin_op = dynamic_cast<BinaryOp*>(initializer)) {
                            switch (bin_op->op) {
                                case BinaryOp::Operator::FloatAdd:
                                case BinaryOp::Operator::FloatSubtract:
                                case BinaryOp::Operator::FloatMultiply:
                                case BinaryOp::Operator::FloatDivide:
                                case BinaryOp::Operator::FloatEqual:
                                case BinaryOp::Operator::FloatNotEqual:
                                case BinaryOp::Operator::FloatLess:
                                case BinaryOp::Operator::FloatLessEqual:
                                case BinaryOp::Operator::FloatGreater:
                                case BinaryOp::Operator::FloatGreaterEqual:
                                    is_float = true;
                                    break;
                                default:
                                    break;
                            }
                        }
                    }
                    determined_type = is_float ? VarType::FLOAT : VarType::INTEGER;
                }
            }
        } else {
            // No initializer; the type is determined solely by the declaration keyword.
            determined_type = node.is_float_declaration ? VarType::FLOAT : VarType::INTEGER;
        }

        // 2. If the variable is a float, transform `HD` operator to its float-specific variant.
        if (determined_type == VarType::FLOAT && initializer) {
            if (auto* un_op = dynamic_cast<UnaryOp*>(initializer)) {
                if (un_op->op == UnaryOp::Operator::HeadOf) {
                    un_op->op = UnaryOp::Operator::HeadOfAsFloat;
                }
            }
        }

        // 3. Update the metrics map for the current function scope.
        auto& metrics = function_metrics_[current_function_scope_];
        metrics.variable_types[name] = determined_type;

        if (determined_type == VarType::FLOAT || determined_type == VarType::POINTER_TO_FLOAT_VEC) {
            metrics.num_float_variables++;
        } else {
            metrics.num_variables++;
        }

        // 4. Update the symbol table, making it the canonical source of type information.
        if (symbol_table_) {
            symbol_table_->addSymbol(name, SymbolKind::LOCAL_VAR, determined_type);
        }

        // 5. Recursively visit the initializer's AST to continue analysis.
        if (initializer) {
            initializer->accept(*this);
        }
    }
}
