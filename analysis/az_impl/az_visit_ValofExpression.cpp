#include "ASTAnalyzer.h"

// Visitor implementation for ValofExpression nodes
void ASTAnalyzer::visit(ValofExpression& node) {
    // Visit the body of the ValofExpression if present
    if (node.body) {
        // Record that we're in a VALOF expression
        std::string previous_function_scope = current_function_scope_;
        
        // Find any RESULTIS statements within the VALOF block
        node.body->accept(*this);
        
        // Check if we need to detect return type from context
        if (!current_function_scope_.empty() && 
            function_return_types_.find(current_function_scope_) != function_return_types_.end()) {
            
            // Check for floating-point operations or literals in the body
            bool has_float_ops = false;
            
            // Look for float expressions in the block
            if (auto* block = dynamic_cast<BlockStatement*>(node.body.get())) {
                for (const auto& stmt : block->statements) {
                    if (auto* resultis = dynamic_cast<ResultisStatement*>(stmt.get())) {
                        if (resultis->expression) {
                            // Check the type of the resultis expression
                            if (auto* num_lit = dynamic_cast<NumberLiteral*>(resultis->expression.get())) {
                                if (num_lit->literal_type == NumberLiteral::LiteralType::Float) {
                                    has_float_ops = true;
                                }
                            }
                            else if (auto* bin_op = dynamic_cast<BinaryOp*>(resultis->expression.get())) {
                                if (bin_op->op == BinaryOp::Operator::FloatAdd || 
                                    bin_op->op == BinaryOp::Operator::FloatSubtract || 
                                    bin_op->op == BinaryOp::Operator::FloatMultiply || 
                                    bin_op->op == BinaryOp::Operator::FloatDivide) {
                                    has_float_ops = true;
                                }
                            }
                            else if (auto* func_call = dynamic_cast<FunctionCall*>(resultis->expression.get())) {
                                if (auto* var_access = dynamic_cast<VariableAccess*>(func_call->function_expr.get())) {
                                    if (var_access->name == "FLOAT") {
                                        has_float_ops = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            
            // Mark this function as returning a float if it contains float operations
            if (has_float_ops) {
                function_return_types_[current_function_scope_] = VarType::FLOAT;
            }
        }
        
        // Restore the previous function scope
        current_function_scope_ = previous_function_scope;
    }
}