#include "CommonSubexpressionEliminationPass.h"
#include "AST.h"
#include "DataTypes.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// --- HELPER: Recursively count all subexpressions in a statement/expression tree (PASS 1) ---
void CommonSubexpressionEliminationPass::count_subexpressions(ASTNode* node) {
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
            for(auto& arg : call->arguments) count_subexpressions(arg.get());
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

// --- Constructor and Temporary Variable Management ---

CommonSubexpressionEliminationPass::CommonSubexpressionEliminationPass(std::unordered_map<std::string, int64_t>& manifests)
    : Optimizer(manifests), temp_var_counter_(0) {
    reset_temporaries();
}

void CommonSubexpressionEliminationPass::reset_temporaries() {
    available_temporaries_ = {"_temp3", "_temp2", "_temp1", "_temp0"};
}

std::string CommonSubexpressionEliminationPass::acquire_temporary() {
    if (available_temporaries_.empty()) {
        std::cerr << "CSE Pass: WARNING - Out of temporary variables." << std::endl;
        return "";
    }
    std::string temp = available_temporaries_.back();
    available_temporaries_.pop_back();
    return temp;
}

void CommonSubexpressionEliminationPass::release_temporary(const std::string& temp_var) {
    available_temporaries_.push_back(temp_var);
}

// --- Main Entry Points for the Two-Pass Strategy ---

void CommonSubexpressionEliminationPass::visit(FunctionDeclaration& node) {
    available_expressions_.clear();
    hoisted_declarations_.clear();
    reset_temporaries();
    expr_counts_.clear();

    if (node.body) count_subexpressions(node.body.get());
    Optimizer::visit(node);

    if (!hoisted_declarations_.empty()) {
        if (auto* compound_body = dynamic_cast<CompoundStatement*>(node.body.get())) {
            for (auto it = hoisted_declarations_.rbegin(); it != hoisted_declarations_.rend(); ++it) {
                compound_body->statements.insert(compound_body->statements.begin(), std::move(*it));
            }
        } else if (auto* block_body = dynamic_cast<BlockStatement*>(node.body.get())) {
            for (auto it = hoisted_declarations_.rbegin(); it != hoisted_declarations_.rend(); ++it) {
                block_body->statements.insert(block_body->statements.begin(), std::move(*it));
            }
        }
    }
}

void CommonSubexpressionEliminationPass::visit(RoutineDeclaration& node) {
    available_expressions_.clear();
    hoisted_declarations_.clear();
    reset_temporaries();
    expr_counts_.clear();

    if (node.body) count_subexpressions(node.body.get());
    Optimizer::visit(node);

    if (!hoisted_declarations_.empty()) {
        if (auto* compound_body = dynamic_cast<CompoundStatement*>(node.body.get())) {
            for (auto it = hoisted_declarations_.rbegin(); it != hoisted_declarations_.rend(); ++it) {
                compound_body->statements.insert(compound_body->statements.begin(), std::move(*it));
            }
        } else if (auto* block_body = dynamic_cast<BlockStatement*>(node.body.get())) {
            for (auto it = hoisted_declarations_.rbegin(); it != hoisted_declarations_.rend(); ++it) {
                block_body->statements.insert(block_body->statements.begin(), std::move(*it));
            }
        }
    }
}

// --- Transformation Logic for Expressions (PASS 2) ---

void CommonSubexpressionEliminationPass::visit(BinaryOp& node) {
    node.left = visit_expr(std::move(node.left));
    node.right = visit_expr(std::move(node.right));
    
    std::string canonical_expr_str = expression_to_string(&node);
    
    auto count_it = expr_counts_.find(canonical_expr_str);
    if (count_it != expr_counts_.end() && count_it->second > 1) {
        auto avail_it = available_expressions_.find(canonical_expr_str);
        if (avail_it != available_expressions_.end()) {
            current_transformed_node_ = std::make_unique<VariableAccess>(avail_it->second);
        } else {
            std::string temp_var_name = acquire_temporary();
            if (!temp_var_name.empty()) {
                available_expressions_[canonical_expr_str] = temp_var_name;
                auto hoisted_expr = std::unique_ptr<Expression>(static_cast<Expression*>(node.clone().release()));
                std::vector<ExprPtr> initializers_vec;
                initializers_vec.push_back(std::move(hoisted_expr));
                hoisted_declarations_.push_back(std::make_unique<LetDeclaration>(
                    std::vector<std::string>{temp_var_name},
                    std::move(initializers_vec)
                ));
                current_transformed_node_ = std::make_unique<VariableAccess>(temp_var_name);
            }
        }
    }
}


// --- Simple Traversal Visitors ---

void CommonSubexpressionEliminationPass::visit(LetDeclaration& node) {
    for (auto& init : node.initializers) {
        init = visit_expr(std::move(init));
    }
}

void CommonSubexpressionEliminationPass::visit(AssignmentStatement& node) {
    for (auto& rhs_expr : node.rhs) {
        rhs_expr = visit_expr(std::move(rhs_expr));
    }
    for (auto& lhs_expr : node.lhs) {
        lhs_expr = visit_expr(std::move(lhs_expr));
    }
}

void CommonSubexpressionEliminationPass::visit(UnaryOp& node) {
    node.operand = visit_expr(std::move(node.operand));
}

void CommonSubexpressionEliminationPass::visit(FunctionCall& node) {
    node.function_expr = visit_expr(std::move(node.function_expr));
    for (auto& arg : node.arguments) {
        arg = visit_expr(std::move(arg));
    }
}

// --- Canonical String Representation ---

std::string CommonSubexpressionEliminationPass::expression_to_string(Expression* expr) {
    if (!expr) return "(NULL)";
    return expression_to_string_recursive(expr);
}

std::string CommonSubexpressionEliminationPass::expression_to_string_recursive(Expression* expr) {
    std::stringstream ss;
    if (!expr) {
        ss << "(NULL_EXPR)";
        return ss.str();
    }
    switch (expr->getType()) {
        case ASTNode::NodeType::NumberLit: {
            auto* num_lit = static_cast<NumberLiteral*>(expr);
            ss << "(NUM_INT " << num_lit->int_value << ")";
            break;
        }
        case ASTNode::NodeType::VariableAccessExpr: {
            auto* var_access = static_cast<VariableAccess*>(expr);
            ss << "(VAR " << var_access->name << ")";
            break;
        }
        case ASTNode::NodeType::BinaryOpExpr: {
            auto* bin_op = static_cast<BinaryOp*>(expr);
            ss << "(BIN_OP " << static_cast<int>(bin_op->op) << " ";
            ss << expression_to_string_recursive(bin_op->left.get()) << " ";
            ss << expression_to_string_recursive(bin_op->right.get()) << ")";
            break;
        }
        default:
            ss << "(UNKNOWN_EXPR_" << reinterpret_cast<uintptr_t>(expr) << ")";
            break;
    }
    return ss.str();
}

// --- Empty/Default Visitor Implementations (to satisfy override requirements) ---

VarType CommonSubexpressionEliminationPass::infer_expression_type(const Expression* expr) { return VarType::INTEGER; }
std::string CommonSubexpressionEliminationPass::generate_temp_var_name() { return ""; }
void CommonSubexpressionEliminationPass::visit(VectorAccess& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(CharIndirection& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(FloatVectorIndirection& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(ConditionalExpression& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(ValofExpression& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(VecAllocationExpression& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(StringAllocationExpression& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(TableExpression& node) { Optimizer::visit(node); }
void CommonSubexpressionEliminationPass::visit(BrkStatement& node) { Optimizer::visit(node); }

void CommonSubexpressionEliminationPass::visit(GlobalVariableDeclaration& node) {
    // Process initializers if they exist
    for (auto& initializer : node.initializers) {
        if (initializer) {
            initializer = visit_expr(std::move(initializer));
        }
    }
}

ExprPtr CommonSubexpressionEliminationPass::visit_expr(ExprPtr expr) {
    if (!expr) return nullptr;
    
    // This helper correctly manages the transformation context for child nodes.
    ASTNodePtr parent_context = std::move(current_transformed_node_);
    current_transformed_node_ = std::move(expr);
    current_transformed_node_->accept(*this);
    ExprPtr result = ExprPtr(static_cast<Expression*>(current_transformed_node_.release()));
    current_transformed_node_ = std::move(parent_context);
    
    return result;
}

