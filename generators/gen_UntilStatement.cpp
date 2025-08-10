#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(UntilStatement& node) {
    debug_print("Visiting UntilStatement node.");
    // `UNTIL condition DO body`
    // This executes the body *until* the condition becomes true.
    // Equivalent to `WHILE NOT condition DO body`.

    std::string loop_label = label_manager_.create_label();
    std::string check_label = label_manager_.create_label(); // Label to jump to for condition check

    // Define loop_label (start of the loop).
    instruction_stream_.define_label(loop_label);

    // Generate code for loop body.
    generate_statement_code(*node.body);

    // Define check_label (where condition is evaluated).
    instruction_stream_.define_label(check_label); // This is where we loop back to.

    // Evaluate condition.
    generate_expression_code(*node.condition);
    std::string cond_reg = expression_result_reg_;

    // Optimization: If the condition is a comparison, use the flags directly and branch if false
    bool used_flags = false;
    if (auto binop = dynamic_cast<BinaryOp*>(node.condition.get())) {
        switch (binop->op) {
            case BinaryOp::Operator::Equal:
                emit(Encoder::create_branch_conditional("NE", loop_label));
                used_flags = true;
                break;
            case BinaryOp::Operator::NotEqual:
                emit(Encoder::create_branch_conditional("EQ", loop_label));
                used_flags = true;
                break;
            case BinaryOp::Operator::Less:
                emit(Encoder::create_branch_conditional("GE", loop_label));
                used_flags = true;
                break;
            case BinaryOp::Operator::LessEqual:
                emit(Encoder::create_branch_conditional("GT", loop_label));
                used_flags = true;
                break;
            case BinaryOp::Operator::Greater:
                emit(Encoder::create_branch_conditional("LE", loop_label));
                used_flags = true;
                break;
            case BinaryOp::Operator::GreaterEqual:
                emit(Encoder::create_branch_conditional("LT", loop_label));
                used_flags = true;
                break;
            default:
                break;
        }
    }

    if (!used_flags) {
        // Fallback: treat cond_reg as boolean (0/1)
        auto& register_manager = register_manager_;
        emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
        emit(Encoder::create_branch_conditional("EQ", loop_label));
        register_manager.release_register(cond_reg);
    }

    // If condition is true, fall through (loop ends).
    debug_print("Finished visiting UntilStatement node.");
}