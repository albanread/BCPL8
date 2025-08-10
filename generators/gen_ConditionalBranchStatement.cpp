#include "NewCodeGenerator.h"
#include "AST.h"

void NewCodeGenerator::visit(ConditionalBranchStatement& node) {
    // 1. Evaluate the condition expression.
    generate_expression_code(*node.condition_expr);
    std::string cond_reg = expression_result_reg_;

    // 2. Compare the result to zero.
    emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
    register_manager_.release_register(cond_reg);

    // 3. Emit the conditional branch.
    emit(Encoder::create_branch_conditional(node.condition, node.targetLabel));

    // Debug information
    debug_print("ConditionalBranchStatement: Emitted CMP and B." + node.condition + " to " + node.targetLabel);
}
