#include "ShortCircuitPass.h"
#include <iostream>

// ShortCircuitPass implementation
// Transforms logical AND and OR operations into conditional expressions
// to enable short-circuit evaluation in BCPL

ProgramPtr ShortCircuitPass::apply(ProgramPtr program) {
    std::cout << "Applying ShortCircuitPass..." << std::endl;
    return Optimizer::apply(std::move(program));
}

void ShortCircuitPass::visit(BinaryOp& node) {
    // First, recursively visit the children of the binary operation.
    // This ensures that nested expressions are transformed first (post-order traversal).
    Optimizer::visit(node);

    // After visiting children, check if the operator is a candidate for short-circuiting.
    
    if (node.op == BinaryOp::Operator::LogicalAnd) {
        // Transform 'A & B' into a conditional expression: 'A ? B : false'
        
        // Move the sub-expressions from the original BinaryOp node.
        ExprPtr condition = std::move(node.left);
        ExprPtr true_expr = std::move(node.right);
        // Create a 'false' literal for the else case.
        ExprPtr false_expr = std::make_unique<BooleanLiteral>(false);

        // Create the new ConditionalExpression node.
        auto conditional_expr = std::make_unique<ConditionalExpression>(
            std::move(condition),
            std::move(true_expr),
            std::move(false_expr)
        );

        // Replace the current BinaryOp node with the new ConditionalExpression.
        // The base Optimizer's visit_expr will handle the replacement.
        current_transformed_node_ = std::move(conditional_expr);

    } else if (node.op == BinaryOp::Operator::LogicalOr) {
        // Transform 'A | B' into a conditional expression: 'A ? true : B'

        // Move the sub-expressions from the original BinaryOp node.
        ExprPtr condition = std::move(node.left);
        // Create a 'true' literal for the then case.
        ExprPtr true_expr = std::make_unique<BooleanLiteral>(true);
        ExprPtr false_expr = std::move(node.right);
        
        // Create the new ConditionalExpression node.
        auto conditional_expr = std::make_unique<ConditionalExpression>(
            std::move(condition),
            std::move(true_expr),
            std::move(false_expr)
        );

        // Replace the current BinaryOp node with the new ConditionalExpression.
        current_transformed_node_ = std::move(conditional_expr);
    }
    // If the operator is not LogicalAnd or LogicalOr, no transformation is needed.
    // The node remains as a BinaryOp (with its children possibly optimized).
}