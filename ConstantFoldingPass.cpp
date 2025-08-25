#include "ConstantFoldingPass.h"
#include <iostream>

// Trace flag for optimizer debug output
static bool trace_optimizer = false;

// --- Helpers for evaluating constant expressions ---

// Evaluates an expression to an integer constant, if possible.
OptionalValue<int64_t> ConstantFoldingPass::evaluate_integer_constant(Expression* expr) const {
    if (!expr) return OptionalValue<int64_t>();
    if (auto num_lit = dynamic_cast<NumberLiteral*>(expr)) {
        if (num_lit->literal_type == NumberLiteral::LiteralType::Integer) {
            return num_lit->int_value;
        }
    }
    if (auto bool_lit = dynamic_cast<BooleanLiteral*>(expr)) {
        return bool_lit->value ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); // BCPL TRUE is -1
    }
    return OptionalValue<int64_t>();
}

// Evaluates an expression to a floating-point constant, if possible.
OptionalValue<double> ConstantFoldingPass::evaluate_float_constant(Expression* expr) const {
    if (!expr) return OptionalValue<double>();
    if (auto num_lit = dynamic_cast<NumberLiteral*>(expr)) {
        if (num_lit->literal_type == NumberLiteral::LiteralType::Float) {
            return num_lit->float_value;
        }
        // Also allow integers to be treated as floats for mixed operations
        if (num_lit->literal_type == NumberLiteral::LiteralType::Integer) {
            return static_cast<double>(num_lit->int_value);
        }
    }
    return OptionalValue<double>();
}

OptionalValue<bool> ConstantFoldingPass::evaluate_boolean_constant(Expression* expr) const {
    if (!expr) {
        return OptionalValue<bool>();
    }
    if (auto bool_lit = dynamic_cast<BooleanLiteral*>(expr)) {
        return bool_lit->value;
    }
    
    auto int_val = evaluate_integer_constant(expr);
    if (int_val.has_value()) {
        return int_val.value() != 0;
    }
    
    auto float_val = evaluate_float_constant(expr);
    if (float_val.has_value()) {
        return float_val.value() != 0.0;
    }
    
    return OptionalValue<bool>();
}

bool ConstantFoldingPass::is_bcpl_true(Expression* expr) const {
    OptionalValue<bool> bool_val = evaluate_boolean_constant(expr);
    if (bool_val.has_value()) {
        return bool_val.value();
    }
    
    auto int_val = evaluate_integer_constant(expr);
    if (int_val.has_value()) {
        return int_val.value() != 0;
    }
    
    auto float_val = evaluate_float_constant(expr);
    if (float_val.has_value()) {
        return float_val.value() != 0.0;
    }
    
    // If it's not a constant literal, we can't determine truthiness at compile time
    return false;
}

ConstantFoldingPass::ConstantFoldingPass(std::unordered_map<std::string, int64_t>& manifests)
    : Optimizer(manifests) {}

ProgramPtr ConstantFoldingPass::apply(ProgramPtr program) {
    program->accept(*this);
    return program;
}

// --- Main Expression Traversal and Folding Logic ---

// Helper to create a NOP (No Operation) statement.
StmtPtr create_nop() {
    // An empty compound statement acts as a NOP and is safe to insert anywhere.
    return std::make_unique<CompoundStatement>(std::vector<StmtPtr>{});
}

// --- Constant Folding and Dead Branch Elimination for Control Flow ---

void ConstantFoldingPass::visit(IfStatement& node) {
    // 1. Fold the condition
    node.condition = visit_expr(std::move(node.condition));

    // 2. Evaluate the condition
    OptionalValue<bool> condition_is_true = evaluate_boolean_constant(node.condition.get());

    if (condition_is_true.has_value()) {
        if (condition_is_true.value()) {
            // Condition is always true. Replace the IF with the (folded) THEN branch.
            std::cout << "[OPTIMIZER] Dead Branch Elimination: IF condition is always TRUE. Replacing with THEN branch." << std::endl;
            current_transformed_node_ = visit_stmt(std::move(node.then_branch));
        } else {
            // Condition is always false. The THEN branch is dead code. Replace the IF with a NOP.
            std::cout << "[OPTIMIZER] Dead Branch Elimination: IF condition is always FALSE. Removing statement." << std::endl;
            current_transformed_node_ = create_nop();
        }
        return;
    }

    // If the condition is not constant, we still need to fold the body.
    node.then_branch = visit_stmt(std::move(node.then_branch));
}

void ConstantFoldingPass::visit(TestStatement& node) {
    // 1. Fold the condition
    node.condition = visit_expr(std::move(node.condition));

    // 2. Evaluate the condition
    OptionalValue<bool> condition_is_true = evaluate_boolean_constant(node.condition.get());

    if (condition_is_true.has_value()) {
        if (condition_is_true.value()) {
            // Condition is always true. Replace TEST with the (folded) THEN branch.
            std::cout << "[OPTIMIZER] Dead Branch Elimination: TEST condition is always TRUE. Replacing with THEN branch." << std::endl;
            current_transformed_node_ = visit_stmt(std::move(node.then_branch));
        } else {
            // Condition is always false. Replace TEST with the (folded) ELSE branch.
            std::cout << "[OPTIMIZER] Dead Branch Elimination: TEST condition is always FALSE. Replacing with ELSE branch." << std::endl;
            if (node.else_branch) {
                current_transformed_node_ = visit_stmt(std::move(node.else_branch));
            } else {
                current_transformed_node_ = create_nop();
            }
        }
        return;
    }

    // If the condition is not constant, fold both branches.
    node.then_branch = visit_stmt(std::move(node.then_branch));
    if (node.else_branch) {
        node.else_branch = visit_stmt(std::move(node.else_branch));
    }
}

void ConstantFoldingPass::visit(WhileStatement& node) {
    // 1. Fold the condition
    node.condition = visit_expr(std::move(node.condition));

    // 2. Evaluate the condition
    OptionalValue<bool> condition_is_true = evaluate_boolean_constant(node.condition.get());

    if (condition_is_true.has_value()) {
        if (!condition_is_true.value()) {
            // Condition is always false. The loop body never executes. Replace WHILE with a NOP.
            std::cout << "[OPTIMIZER] Dead Branch Elimination: WHILE condition is always FALSE. Removing loop." << std::endl;
            current_transformed_node_ = create_nop();
            return;
        }
        // If condition is always TRUE, it's an infinite loop. We leave it, but the condition is now simplified.
    }

    // Fold the loop body.
    node.body = visit_stmt(std::move(node.body));
}

// --- Main Expression Traversal and Folding Logic ---





// --- Specific Visitor Implementations for Folding ---

void ConstantFoldingPass::visit(NumberLiteral& node) {
    // Base case: NumberLiterals are already constants. No folding needed.
}

void ConstantFoldingPass::visit(BooleanLiteral& node) {
    // Base case: BooleanLiterals are already constants. No folding needed.
}

void ConstantFoldingPass::visit(BinaryOp& node) {
    // First, recursively fold the left and right sub-trees.
    node.left = visit_expr(std::move(node.left));
    node.right = visit_expr(std::move(node.right));

    // Attempt integer folding first.
    auto left_int = evaluate_integer_constant(node.left.get());
    auto right_int = evaluate_integer_constant(node.right.get());

    if (left_int.has_value() && right_int.has_value()) {
        int64_t result;
        bool foldable = true;
        switch (node.op) {
            case BinaryOp::Operator::Add: result = left_int.value() + right_int.value(); break;
            case BinaryOp::Operator::Subtract: result = left_int.value() - right_int.value(); break;
            case BinaryOp::Operator::Multiply: result = left_int.value() * right_int.value(); break;
            case BinaryOp::Operator::Divide:
                if (right_int.value() == 0) foldable = false; else result = left_int.value() / right_int.value();
                break;
            case BinaryOp::Operator::Remainder:
                if (right_int.value() == 0) foldable = false; else result = left_int.value() % right_int.value();
                break;
            case BinaryOp::Operator::LeftShift: result = left_int.value() << right_int.value(); break;
            case BinaryOp::Operator::RightShift: result = left_int.value() >> right_int.value(); break;
            case BinaryOp::Operator::Equal: result = (left_int.value() == right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::NotEqual: result = (left_int.value() != right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::Less: result = (left_int.value() < right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::LessEqual: result = (left_int.value() <= right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::Greater: result = (left_int.value() > right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::GreaterEqual: result = (left_int.value() >= right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::LogicalAnd: result = (left_int.value() != 0 && right_int.value() != 0) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::LogicalOr: result = (left_int.value() != 0 || right_int.value() != 0) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::BitwiseOr: result = left_int.value() | right_int.value(); break;
            case BinaryOp::Operator::Equivalence: result = (left_int.value() == right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::NotEquivalence: result = (left_int.value() != right_int.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            default: foldable = false; break;
        }
        if (foldable) {
            current_transformed_node_ = std::make_unique<NumberLiteral>(result);
            if (trace_optimizer) {
                std::cout << "[OPTIMIZER] Folded BinaryOp to NumberLiteral: " << result << std::endl;
            }
            return;
        }
    }

    // --- Algebraic Simplification Logic ---
    // Identity: X + 0 => X  or  0 + X => X
    if (node.op == BinaryOp::Operator::Add) {
        if (left_int.has_value() && left_int.value() == 0) {
            current_transformed_node_ = std::move(node.right); // Replace with right side
            return;
        }
        if (right_int.has_value() && right_int.value() == 0) {
            current_transformed_node_ = std::move(node.left); // Replace with left side
            return;
        }
    }

    // Identity: X - 0 => X
    if (node.op == BinaryOp::Operator::Subtract) {
        if (right_int.has_value() && right_int.value() == 0) {
            current_transformed_node_ = std::move(node.left); // Replace with left side
            return;
        }
    }

    // Identity: X * 1 => X  or  1 * X => X
    if (node.op == BinaryOp::Operator::Multiply) {
        if (left_int.has_value() && left_int.value() == 1) {
            current_transformed_node_ = std::move(node.right); // Replace with right side
            return;
        }
        if (right_int.has_value() && right_int.value() == 1) {
            current_transformed_node_ = std::move(node.left); // Replace with left side
            return;
        }
        // X * 0 => 0 or 0 * X => 0
        if ((left_int.has_value() && left_int.value() == 0) ||
            (right_int.has_value() && right_int.value() == 0)) {
            current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<int64_t>(0));
            return;
        }
    }

    // Identity: X / 1 => X
    if (node.op == BinaryOp::Operator::Divide) {
        if (right_int.has_value() && right_int.value() == 1) {
            current_transformed_node_ = std::move(node.left); // Replace with left side
            return;
        }
    }

    // Identity: X - X => 0
    if (node.op == BinaryOp::Operator::Subtract) {
        if (left_int.has_value() && right_int.has_value() && left_int.value() == right_int.value()) {
            current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<int64_t>(0));
            return;
        }
    }

    // If integer folding fails, attempt floating-point folding.
    auto left_float = evaluate_float_constant(node.left.get());
    auto right_float = evaluate_float_constant(node.right.get());

    if (left_float.has_value() && right_float.has_value()) {
        double result;
        int64_t bool_result; // For float comparisons
        bool is_comparison = false;
        bool foldable = true;
        switch (node.op) {
            case BinaryOp::Operator::FloatAdd: result = left_float.value() + right_float.value(); break;
            case BinaryOp::Operator::FloatSubtract: result = left_float.value() - right_float.value(); break;
            case BinaryOp::Operator::FloatMultiply: result = left_float.value() * right_float.value(); break;
            case BinaryOp::Operator::FloatDivide:
                if (right_float.value() == 0.0) foldable = false; else result = left_float.value() / right_float.value();
                break;
            case BinaryOp::Operator::FloatEqual: is_comparison = true; bool_result = (left_float.value() == right_float.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::FloatNotEqual: is_comparison = true; bool_result = (left_float.value() != right_float.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::FloatLess: is_comparison = true; bool_result = (left_float.value() < right_float.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::FloatLessEqual: is_comparison = true; bool_result = (left_float.value() <= right_float.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::FloatGreater: is_comparison = true; bool_result = (left_float.value() > right_float.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            case BinaryOp::Operator::FloatGreaterEqual: is_comparison = true; bool_result = (left_float.value() >= right_float.value()) ? static_cast<int64_t>(-1) : static_cast<int64_t>(0); break;
            default: foldable = false; break;
        }
        if (foldable) {
            if (is_comparison) {
                current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<int64_t>(bool_result));
                std::cout << "[OPTIMIZER] Folded Float comparison to NumberLiteral: " << bool_result << std::endl;
            } else {
                current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<double>(result));
                std::cout << "[OPTIMIZER] Folded Float BinaryOp to NumberLiteral: " << result << std::endl;
            }
            return;
        }
    }
}

void ConstantFoldingPass::visit(UnaryOp& node) {
    node.operand = visit_expr(std::move(node.operand));

    auto int_val = evaluate_integer_constant(node.operand.get());
    if (int_val.has_value()) {
        switch (node.op) {
            case UnaryOp::Operator::Negate:
                current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<int64_t>(-(int_val.value())));
                std::cout << "[OPTIMIZER] Folded UnaryOp to NumberLiteral: " << -(int_val.value()) << std::endl;
                return;
            case UnaryOp::Operator::LogicalNot:
                current_transformed_node_ = std::make_unique<NumberLiteral>(int_val.value() == 0 ? static_cast<int64_t>(-1) : static_cast<int64_t>(0));
                std::cout << "[OPTIMIZER] Folded LogicalNot to NumberLiteral: " << (int_val.value() == 0 ? -1 : 0) << std::endl;
                return;
            case UnaryOp::Operator::TailOfNonDestructive:
                // REST is not a constant-foldable operation, but we should not crash.
                break;
            default: break;
        }
    }

    auto float_val = evaluate_float_constant(node.operand.get());
    if (float_val.has_value()) {
         if (node.op == UnaryOp::Operator::Negate) {
            current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<double>(-(float_val.value())));
            std::cout << "[OPTIMIZER] Folded Float UnaryOp to NumberLiteral: " << -(float_val.value()) << std::endl;
            return;
        }
    }
}

void ConstantFoldingPass::visit(ConditionalExpression& node) {
    node.condition = visit_expr(std::move(node.condition));

    OptionalValue<bool> condition_is_true = evaluate_boolean_constant(node.condition.get());

    if (condition_is_true.has_value()) {
        if (condition_is_true.value()) {
            current_transformed_node_ = visit_expr(std::move(node.true_expr));
            std::cout << "[OPTIMIZER] Folded ConditionalExpression (condition true) to true_expr." << std::endl;
        } else {
            current_transformed_node_ = visit_expr(std::move(node.false_expr));
            std::cout << "[OPTIMIZER] Folded ConditionalExpression (condition false) to false_expr." << std::endl;
        }
        return;
    }

    node.true_expr = visit_expr(std::move(node.true_expr));
    node.false_expr = visit_expr(std::move(node.false_expr));
}

// --- Other visit methods (default traversal from Optimizer base class) ---
void ConstantFoldingPass::visit(FunctionDeclaration& node) {
    known_constants_.clear(); // Reset for the new function scope.
    Optimizer::visit(node);   // Continue with normal traversal.
}

void ConstantFoldingPass::visit(RoutineDeclaration& node) {
    known_constants_.clear(); // Reset for the new routine scope.
    Optimizer::visit(node);   // Continue with normal traversal.
}

void ConstantFoldingPass::visit(VecAllocationExpression& node) {}

void ConstantFoldingPass::visit(ValofExpression& node) {
    if (node.body) {
        node.body = visit_stmt(std::move(node.body));
    }
}

void ConstantFoldingPass::visit(FloatValofExpression& node) {
    if (node.body) {
        node.body = visit_stmt(std::move(node.body));
    }
}
void ConstantFoldingPass::visit(StringAllocationExpression& node) {}
void ConstantFoldingPass::visit(TableExpression& node) {}
void ConstantFoldingPass::visit(BrkStatement& node) {}
void ConstantFoldingPass::visit(GlobalVariableDeclaration& node) {
    for (auto& initializer : node.initializers) {
        if (initializer) {
            initializer = visit_expr(std::move(initializer));
        }
    }
}

void ConstantFoldingPass::visit(VariableAccess& node) {
    // 1. Check our new map of known local constants first.
    auto it_local = known_constants_.find(node.name);
    if (it_local != known_constants_.end()) {
        // This variable has a known constant value. Replace this node
        // with a NumberLiteral containing that value.
        current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<int64_t>(it_local->second));
        if (trace_optimizer) {
            std::cout << "[OPTIMIZER] Propagated constant for variable '" << node.name 
                      << "' with value " << it_local->second << std::endl;
        }
        return; // Important: exit after transformation
    }
    // 2. Keep the existing logic for MANIFEST constants as a fallback.
    auto it = manifests_.find(node.name);
    if (it != manifests_.end()) {
        current_transformed_node_ = std::make_unique<NumberLiteral>(static_cast<int64_t>(it->second));
        std::cout << "[OPTIMIZER] Folded VariableAccess '" << node.name << "' (MANIFEST) to NumberLiteral: " << it->second << std::endl;
    }
}

void ConstantFoldingPass::visit(AssignmentStatement& node) {
    // First, recursively optimize the right-hand side of the assignment.
    for (auto& rhs_expr : node.rhs) {
        rhs_expr = visit_expr(std::move(rhs_expr));
    }

    // After folding, check if the RHS is a constant literal.
    // This example handles single assignments like 'x := 10'.
    if (node.lhs.size() == 1 && node.rhs.size() == 1) {
        auto* lhs_var = dynamic_cast<VariableAccess*>(node.lhs[0].get());
        auto* rhs_lit = dynamic_cast<NumberLiteral*>(node.rhs[0].get());

        if (lhs_var) {
            if (rhs_lit && rhs_lit->literal_type == NumberLiteral::LiteralType::Integer) {
                // The variable 'lhs_var->name' is now a known constant. Track it.
                known_constants_[lhs_var->name] = rhs_lit->int_value;
                if (trace_optimizer) {
                    std::cout << "[OPTIMIZER] Propagation: Variable '" << lhs_var->name 
                              << "' is now constant with value " << rhs_lit->int_value << std::endl;
                }
            } else {
                // The variable is being assigned a non-constant value.
                // We must remove it from our tracking map if it was there before.
                known_constants_.erase(lhs_var->name);
            }
        }
    }

    // DO NOT visit the LHS for any expressions. The LHS is a target,
    // not a value to be folded. The loop that was here has been removed.
}

void ConstantFoldingPass::visit(ForStatement& node) {
    node.start_expr = visit_expr(std::move(node.start_expr));
    node.end_expr = visit_expr(std::move(node.end_expr));
    if (node.step_expr) {
        node.step_expr = visit_expr(std::move(node.step_expr));
    }
    node.body = visit_stmt(std::move(node.body));
}

void ConstantFoldingPass::visit(RepeatStatement& node) {
    node.body = visit_stmt(std::move(node.body));
    if (node.condition) {
        node.condition = visit_expr(std::move(node.condition));
    }
}