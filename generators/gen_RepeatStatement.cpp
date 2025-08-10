#include "../NewCodeGenerator.h"
#include "../LabelManager.h"
#include "../analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

// This helper function correctly determines the branch condition.
static std::string get_branch_condition_for_repeat(RepeatStatement::LoopType loop_type, BinaryOp::Operator op) {
    // REPEAT WHILE loops as long as the condition is TRUE.
    // REPEAT UNTIL loops as long as the condition is FALSE.
    bool loop_while_true = (loop_type == RepeatStatement::LoopType::RepeatWhile);

    switch (op) {
        case BinaryOp::Operator::Equal:        return loop_while_true ? "EQ" : "NE";
        case BinaryOp::Operator::NotEqual:     return loop_while_true ? "NE" : "EQ";
        case BinaryOp::Operator::Less:         return loop_while_true ? "LT" : "GE";
        case BinaryOp::Operator::LessEqual:    return loop_while_true ? "LE" : "GT";
        case BinaryOp::Operator::Greater:      return loop_while_true ? "GT" : "LE";
        case BinaryOp::Operator::GreaterEqual: return loop_while_true ? "GE" : "LT";
        default: return ""; // Not a direct comparison
    }
}

void NewCodeGenerator::visit(RepeatStatement& node) {
    debug_print("Visiting RepeatStatement node (NOTE: branching is handled by block epilogue).");
    // This visitor is now intentionally empty.
    // The CFGBuilderPass has already separated the body and condition into
    // distinct basic blocks, which are handled by the main codegen loop
    // and the generate_block_epilogue function.
}
