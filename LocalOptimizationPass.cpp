#include "LocalOptimizationPass.h"
#include "BasicBlock.h"
#include "AST.h"
#include <unordered_set>
#include <sstream>
#include <algorithm>
#include <iostream>
#include "SymbolTable.h"
#include "analysis/ASTAnalyzer.h"
#include "DataTypes.h"

// --- Type inference for expressions ---
VarType LocalOptimizationPass::infer_expression_type(const Expression* expr) {
    if (!expr) return VarType::INTEGER; // Default to integer

    if (const auto* bin_op = dynamic_cast<const BinaryOp*>(expr)) {
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
                return VarType::FLOAT;
            default:
                // Recurse on children to see if they produce floats
                if (infer_expression_type(bin_op->left.get()) == VarType::FLOAT ||
                    infer_expression_type(bin_op->right.get()) == VarType::FLOAT) {
                    return VarType::FLOAT;
                }
                return VarType::INTEGER;
        }
    }
    if (const auto* un_op = dynamic_cast<const UnaryOp*>(expr)) {
        if (un_op->op == UnaryOp::Operator::FloatConvert) {
            return VarType::FLOAT;
        }
        // You may want to recurse for other unary ops if needed
    }
    if (dynamic_cast<const NumberLiteral*>(expr)) {
        const auto* lit = static_cast<const NumberLiteral*>(expr);
        if (lit->literal_type == NumberLiteral::LiteralType::Float) {
            return VarType::FLOAT;
        }
    }
    // TODO: Add logic for FunctionCall, VariableAccess, etc. if needed
    return VarType::INTEGER;
}

// --- Helper: Canonical string for an expression (for CSE) ---
// This is a simplified version; you may want to expand it for more expression types.
static std::string expression_to_string_recursive(const Expression* expr);

static std::string expression_to_string(const Expression* expr) {
    if (!expr) return "";
    return expression_to_string_recursive(expr);
}

static std::string expression_to_string_recursive(const Expression* expr) {
    if (!expr) return "";
    switch (expr->getType()) {
        case ASTNode::NodeType::BinaryOpExpr: {
            const auto* bin = static_cast<const BinaryOp*>(expr);
            std::string op_str = std::to_string(static_cast<int>(bin->op));
            std::string left_str = expression_to_string(bin->left.get());
            std::string right_str = expression_to_string(bin->right.get());

            // Canonicalization for commutative operators
            switch (bin->op) {
                case BinaryOp::Operator::Add:
                case BinaryOp::Operator::Multiply:
                case BinaryOp::Operator::LogicalAnd:
                case BinaryOp::Operator::LogicalOr:
                case BinaryOp::Operator::Equal:
                case BinaryOp::Operator::NotEqual:
                    // Add other commutative float ops as needed
                    if (left_str > right_str) {
                        std::swap(left_str, right_str);
                    }
                    break;
                default:
                    // Not a commutative operator, order matters.
                    break;
            }
            return "(BIN_OP " + op_str + " " + left_str + " " + right_str + ")";
        }
        case ASTNode::NodeType::VariableAccessExpr: {
            const auto* var = static_cast<const VariableAccess*>(expr);
            return "(VAR " + var->name + ")";
        }
        case ASTNode::NodeType::NumberLit: {
            const auto* lit = static_cast<const NumberLiteral*>(expr);
            if (lit->literal_type == NumberLiteral::LiteralType::Integer) {
                return "(INT " + std::to_string(lit->int_value) + ")";
            } else {
                return "(FLOAT " + std::to_string(lit->float_value) + ")";
            }
        }
        case ASTNode::NodeType::StringLit: {
            const auto* lit = static_cast<const StringLiteral*>(expr);
            return "(STR \"" + lit->value + "\")";
        }
        case ASTNode::NodeType::CharLit: {
            const auto* lit = static_cast<const CharLiteral*>(expr);
            return "(CHAR " + std::to_string(lit->value) + ")";
        }
        case ASTNode::NodeType::BooleanLit: {
            const auto* lit = static_cast<const BooleanLiteral*>(expr);
            return lit->value ? "(BOOL 1)" : "(BOOL 0)";
        }
        // Add more cases as needed for your language
        default:
            return "(EXPR)";
    }
}

// --- LocalOptimizationPass Implementation ---

LocalOptimizationPass::LocalOptimizationPass()
    : temp_var_counter_(0)
{}

std::string LocalOptimizationPass::generate_temp_var_name() {
    return "_cse_temp_" + std::to_string(temp_var_counter_++);
}

void LocalOptimizationPass::run(const std::unordered_map<std::string, std::unique_ptr<ControlFlowGraph>>& cfgs,
                                SymbolTable& symbol_table,
                                ASTAnalyzer& analyzer) {
    for (const auto& cfg_pair : cfgs) {
        const auto& cfg = cfg_pair.second;
        const std::string& function_name = cfg->function_name;
        for (const auto& block_pair : cfg->get_blocks()) {
            BasicBlock* block = block_pair.second.get();

            // --- PASS 1: ANALYSIS (NEW) ---
            expr_counts_.clear(); // Reset for each block
            for (const auto& stmt : block->statements) {
                count_subexpressions(stmt.get());
            }

            // --- PASS 2: TRANSFORMATION (EXISTING) ---
            available_expressions_.clear();
            temp_var_counter_ = 0; // Optionally reset per block, or keep global for unique names
            optimize_basic_block(block, function_name, symbol_table, analyzer);
        }
    }
}

void LocalOptimizationPass::optimize_basic_block(BasicBlock* block,
                                                const std::string& function_name,
                                                SymbolTable& symbol_table,
                                                ASTAnalyzer& analyzer) {
    // Use an index-based loop to allow for statement insertion.
    for (size_t i = 0; i < block->statements.size(); ++i) {
        // Pass the block and the current index to the optimizer function.
        optimize_assignment(block, i, function_name, symbol_table, analyzer);
    }
}

void LocalOptimizationPass::optimize_assignment(BasicBlock* block, size_t& i,
                                               const std::string& current_function_name,
                                               SymbolTable& symbol_table,
                                               ASTAnalyzer& analyzer) {
    StmtPtr& stmt_ptr = block->statements[i];
    if (!stmt_ptr || stmt_ptr->getType() != ASTNode::NodeType::AssignmentStmt) {
        return;
    }

    auto* assign = static_cast<AssignmentStatement*>(stmt_ptr.get());

    // --- CSE Logic for Right-Hand Side Expressions ---
    for (size_t j = 0; j < assign->rhs.size(); ++j) {
        ExprPtr& rhs_expr = assign->rhs[j];
        if (!rhs_expr) continue;

        std::string canonical_expr_str = expression_to_string(rhs_expr.get());

        // If the expression is already available, just replace it.
        auto it = available_expressions_.find(canonical_expr_str);
        if (it != available_expressions_.end()) {
            rhs_expr = std::make_unique<VariableAccess>(it->second);
        }
        // Otherwise, only hoist if it's a common subexpression (count > 1)
        else if (
            rhs_expr->getType() == ASTNode::NodeType::BinaryOpExpr &&
            expr_counts_.count(canonical_expr_str) &&
            expr_counts_.at(canonical_expr_str) > 1
        ) {
            // 1. Generate the temporary variable name and store it.
            std::string temp_var_name = generate_temp_var_name();
            available_expressions_[canonical_expr_str] = temp_var_name;

            // --- Infer the type of the expression being hoisted ---
            VarType inferred_type = infer_expression_type(rhs_expr.get());

            // --- Register the new variable in the Symbol Table for the current function scope ---
            symbol_table.addSymbol(
                temp_var_name,
                SymbolKind::LOCAL_VAR,
                inferred_type
            );

            // --- Increment the local variable count in the ASTAnalyzer's metrics ---
            auto& metrics = analyzer.get_function_metrics_mut().at(current_function_name);
            if (inferred_type == VarType::FLOAT) {
                metrics.num_float_variables++;
            } else {
                metrics.num_variables++;
            }
            metrics.variable_types[temp_var_name] = inferred_type;

            // 2. Create the new "LET" statement to be inserted.
            //    The initializer takes ownership of the expression from the original statement.
            std::vector<ExprPtr> inits;
            inits.push_back(std::move(rhs_expr));
            auto let_decl = std::make_unique<LetDeclaration>(
                std::vector<std::string>{temp_var_name},
                std::move(inits)
            );

            // 3. Replace the original expression with a variable access to our new temp.
            assign->rhs[j] = std::make_unique<VariableAccess>(temp_var_name);

            // 4. Insert the new LET statement *before* the current assignment statement.
            block->statements.insert(block->statements.begin() + i, std::move(let_decl));

            // 5. CRITICAL: Increment the loop index to skip over the new statement we just added.
            i++;
        }
    }

    // --- Invalidation Logic (this part is already correct) ---
    for (const auto& lhs_expr : assign->lhs) {
        if (lhs_expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(lhs_expr.get());
            invalidate_expressions_with_var(var->name);
        }
    }
}

void LocalOptimizationPass::invalidate_expressions_with_var(const std::string& var_name) {
    std::string var_str = "(VAR " + var_name + ")";
    for (auto it = available_expressions_.begin(); it != available_expressions_.end(); ) {
        if (it->first.find(var_str) != std::string::npos) {
            it = available_expressions_.erase(it);
        } else {
            ++it;
        }
    }
}

// (Optional) If you want to expose expression_to_string as a member:
std::string LocalOptimizationPass::expression_to_string(const Expression* expr) const {
    if (!expr) return "";
    return expression_to_string_recursive(expr);
}

// --- NEW: Helper to recursively count all subexpressions in a statement/expression tree (ANALYSIS STAGE) ---
void LocalOptimizationPass::count_subexpressions(ASTNode* node) {
    if (!node) return;

    if (auto* expr = dynamic_cast<Expression*>(node)) {
        std::string key = expression_to_string(expr);
        if (expr->getType() == ASTNode::NodeType::BinaryOpExpr) {
            expr_counts_[key]++;
        }

        if (auto* bin_op = dynamic_cast<BinaryOp*>(expr)) {
            count_subexpressions(bin_op->left.get());
            count_subexpressions(bin_op->right.get());
        } else if (auto* un_op = dynamic_cast<UnaryOp*>(expr)) {
            count_subexpressions(un_op->operand.get());
        } else if (auto* call = dynamic_cast<FunctionCall*>(expr)) {
            count_subexpressions(call->function_expr.get());
            for (auto& arg : call->arguments) count_subexpressions(arg.get());
        }
    }
    else if (auto* let = dynamic_cast<LetDeclaration*>(node)) {
        for (auto& init : let->initializers) count_subexpressions(init.get());
    } else if (auto* assign = dynamic_cast<AssignmentStatement*>(node)) {
        for (auto& rhs : assign->rhs) count_subexpressions(rhs.get());
    } else if (auto* block = dynamic_cast<BlockStatement*>(node)) {
        for (auto& s : block->statements) count_subexpressions(s.get());
    } else if (auto* comp = dynamic_cast<CompoundStatement*>(node)) {
        for (auto& s : comp->statements) count_subexpressions(s.get());
    } else if (auto* if_stmt = dynamic_cast<IfStatement*>(node)) {
        count_subexpressions(if_stmt->condition.get());
        count_subexpressions(if_stmt->then_branch.get());
    }
}