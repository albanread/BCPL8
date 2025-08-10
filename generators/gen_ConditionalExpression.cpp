#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(ConditionalExpression& node) {
    debug_print("Visiting ConditionalExpression node.");
    // `condition ? true_expr : false_expr`
    // Generate code for condition.
    generate_expression_code(*node.condition);
    std::string cond_reg = expression_result_reg_; // Result of condition (0 for false, 1 for true)

    // Generate labels for the branches.
    std::string else_label = label_manager_.create_label();
    std::string end_if_label = label_manager_.create_label();

    // If condition is 0 (false), branch to else_label.
    // CMP cond_reg, #0
    // B.EQ else_label
    emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
    emit(Encoder::create_branch_conditional("EQ", else_label));
    register_manager_.release_register(cond_reg); // Release condition register *after* CMP/branch

    // Generate code for the true_expr.
    generate_expression_code(*node.true_expr);
    std::string true_result_reg = expression_result_reg_; // Register with true_expr result

    // Allocate a register for the final result of the conditional expression.
    std::string final_result_reg = register_manager_.get_free_register();
    if (final_result_reg.empty()) {
        throw std::runtime_error("Failed to acquire a free register for conditional expression result");
    }

    // Move the result of the true_expr into the final_result_reg.
    emit(Encoder::create_mov_reg(final_result_reg, true_result_reg));
    register_manager_.release_register(true_result_reg);

    // Unconditional branch to else_label after true_expr.
    emit(Encoder::create_branch_unconditional(end_if_label));

    // Define else_label.
    instruction_stream_.define_label(else_label);

    // Generate code for the false_expr.
    generate_expression_code(*node.false_expr);
    std::string false_result_reg = expression_result_reg_; // Register with false_expr result

    // Move the result of the false_expr into the final_result_reg.
    emit(Encoder::create_mov_reg(final_result_reg, false_result_reg));
    register_manager_.release_register(false_result_reg);

    // Define end_if_label.
    instruction_stream_.define_label(end_if_label);

    expression_result_reg_ = final_result_reg;
    debug_print("Finished visiting ConditionalExpression node.");
}