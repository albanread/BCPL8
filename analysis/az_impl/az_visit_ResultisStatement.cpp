#include "ASTAnalyzer.h"

// Visitor implementation for ResultisStatement nodes
void ASTAnalyzer::visit(ResultisStatement& node) {
    if (node.expression) {
        node.expression->accept(*this);
        
        // Determine if this is a floating-point result
        bool returns_float = false;
        
        // Check if the expression is a floating point literal
        if (auto* num_lit = dynamic_cast<NumberLiteral*>(node.expression.get())) {
            if (num_lit->literal_type == NumberLiteral::LiteralType::Float) {
                returns_float = true;
            }
        }
        // Check if the expression is a float operation
        else if (auto* bin_op = dynamic_cast<BinaryOp*>(node.expression.get())) {
            if (bin_op->op == BinaryOp::Operator::FloatAdd || 
                bin_op->op == BinaryOp::Operator::FloatSubtract || 
                bin_op->op == BinaryOp::Operator::FloatMultiply || 
                bin_op->op == BinaryOp::Operator::FloatDivide) {
                returns_float = true;
            }
        }
        // Check if expression is a FLOAT() call
        else if (auto* func_call = dynamic_cast<FunctionCall*>(node.expression.get())) {
            if (auto* var_access = dynamic_cast<VariableAccess*>(func_call->function_expr.get())) {
                if (var_access->name == "FLOAT") {
                    returns_float = true;
                }
            }
        }
        
        // Set the current function's return type if it's in a ValofExpression
        if (returns_float && !current_function_scope_.empty()) {
            function_return_types_[current_function_scope_] = VarType::FLOAT;
        }
    }
}