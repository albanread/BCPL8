#include "../ASTAnalyzer.h"
#include "../../SymbolTable.h"


void ASTAnalyzer::visit(LetDeclaration& node) {
    for (size_t i = 0; i < node.names.size(); ++i) {
        const std::string& name = node.names[i];
        variable_definitions_[name] = current_lexical_scope_;
        // Check if we can use the symbol table for information
        bool is_float = node.is_float_declaration;
        
        if (symbol_table_) {
            Symbol symbol;
            if (symbol_table_->lookup(name, symbol)) {
                is_float = (symbol.type == VarType::FLOAT);
            }
        }

        // The variable's metrics (like count) are attributed to the CURRENT FUNCTION scope.
        if (current_function_scope_ != "Global") {
            // START of modified logic
            
            // If we didn't get info from the symbol table, prioritize the FLET flag
            Symbol temp_symbol;
            if (!symbol_table_ || !symbol_table_->lookup(name, temp_symbol)) {
                // 1. Prioritize the FLET flag from the parser.
                is_float = node.is_float_declaration;

            // 2. If it's not an FLET, then try to infer the type from the initializer.
            if (!is_float && i < node.initializers.size() && node.initializers[i]) {
                Expression* initializer = node.initializers[i].get();
                
                // Rule 1: Initializer is a float literal
                if (auto* num_lit = dynamic_cast<NumberLiteral*>(initializer)) {
                    if (num_lit->literal_type == NumberLiteral::LiteralType::Float) {
                        is_float = true;
                    }
                }
                // Rule 2: Initializer is a call to a known float function
                else if (auto* func_call = dynamic_cast<FunctionCall*>(initializer)) {
                    if (auto* var_access = dynamic_cast<VariableAccess*>(func_call->function_expr.get())) {
                        const std::string& func_name = var_access->name;
                        // Check both user-defined float functions AND runtime float functions.
                        if (is_local_float_function(func_name) || (get_runtime_function_type(func_name) == FunctionType::FLOAT)) {
                            is_float = true;
                        }
                    }
                }
                // Rule 3: Initializer is a float-producing binary operation
                else if (auto* bin_op = dynamic_cast<BinaryOp*>(initializer)) {
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
                            // Not a float operation
                            break;
                    }
                }
            }

            }
            
            if (is_float) {
                function_metrics_[current_function_scope_].num_float_variables++;
                function_metrics_[current_function_scope_].variable_types[name] = VarType::FLOAT;
            } else {
                function_metrics_[current_function_scope_].num_variables++;
                function_metrics_[current_function_scope_].variable_types[name] = VarType::INTEGER;
            }
            // END of modified logic
        }
        
        // No longer need to set CallFrameManager here, now handled in NewCodeGenerator

        if (i < node.initializers.size() && node.initializers[i]) {
            node.initializers[i]->accept(*this);
        }
    }
}