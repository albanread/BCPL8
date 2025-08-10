#include "ManifestResolutionPass.h"
#include <iostream>
#include <algorithm>
#include <sstream>

// ManifestResolutionPass constructor and apply() in global namespace

ManifestResolutionPass::ManifestResolutionPass(std::unordered_map<std::string, int64_t>& global_manifests)
    : global_manifests_(global_manifests), current_scope_name_("Global") {
    current_scope_manifests_stack_.push({});
}

ProgramPtr ManifestResolutionPass::apply(ProgramPtr program) {
    // 1. Reset state as before.
    global_manifests_.clear();
    while (!current_scope_manifests_stack_.empty()) {
         current_scope_manifests_stack_.pop();
    }
    current_scope_manifests_stack_.push({});
    current_scope_name_ = "Global";

    // 2. Visit the AST directly. Do NOT move it into a member variable.
    //    The visitor will modify the tree that 'program' points to.
    if (program) {
        program->accept(*this);
    }
    
    // 3. Return the original pointer, which now points to the transformed tree.
    return program;
}



ExprPtr ManifestResolutionPass::visit_expr(ExprPtr expr) {
    if (!expr) return nullptr;

    // Save the parent's transformation context
    ASTNodePtr parent_context = std::move(current_transformed_node_);

    current_transformed_node_ = std::move(expr);
    current_transformed_node_->accept(*this);
    ExprPtr result = ExprPtr(static_cast<Expression*>(current_transformed_node_.release()));

    // Restore the parent's transformation context
    current_transformed_node_ = std::move(parent_context);
    
    return result;
}

StmtPtr ManifestResolutionPass::visit_stmt(StmtPtr stmt) {
    if (!stmt) return nullptr;
    ASTNodePtr parent_context = std::move(current_transformed_node_);
    current_transformed_node_ = std::move(stmt);
    current_transformed_node_->accept(*this);
    StmtPtr result = StmtPtr(static_cast<Statement*>(current_transformed_node_.release()));
    current_transformed_node_ = std::move(parent_context);
    return result;
}

DeclPtr ManifestResolutionPass::visit_decl(DeclPtr decl) {
    if (!decl) return nullptr;
    ASTNodePtr parent_context = std::move(current_transformed_node_);
    current_transformed_node_ = std::move(decl);
    current_transformed_node_->accept(*this);
    DeclPtr result = DeclPtr(static_cast<Declaration*>(current_transformed_node_.release()));
    current_transformed_node_ = std::move(parent_context);
    return result;
}

std::optional<int64_t> ManifestResolutionPass::get_manifest_value(const std::string& name) const {
    if (!current_scope_manifests_stack_.empty()) {
        const auto& top_scope = current_scope_manifests_stack_.top();
        auto it = top_scope.find(name);
        if (it != top_scope.end()) {
            return it->second;
        }
    }
    auto it = global_manifests_.find(name);
    if (it != global_manifests_.end()) {
        return it->second;
    }
    return std::nullopt;
}

// --- Visitor Implementations ---

void ManifestResolutionPass::visit(Program& node) {
    // --- SAFE PATTERN FOR DECLARATIONS ---
    std::vector<DeclPtr> new_declarations;
    new_declarations.reserve(node.declarations.size()); // Pre-allocate memory

    for (auto& decl : node.declarations) {
        if (decl) {
            // Move the original node out of the old vector to transform it
            DeclPtr transformed_decl = visit_decl(std::move(decl));

            // If the transformation didn't remove the node (by returning null),
            // add it to our new, clean list.
            if (transformed_decl) {
                new_declarations.push_back(std::move(transformed_decl));
            }
        }
    }
    // Atomically swap the old, messy vector with our new, correct one.
    node.declarations = std::move(new_declarations);

    // --- SAFE PATTERN FOR STATEMENTS ---
    std::vector<StmtPtr> new_statements;
    new_statements.reserve(node.statements.size());

    for (auto& stmt : node.statements) {
        if (stmt) {
            StmtPtr transformed_stmt = visit_stmt(std::move(stmt));
            if (transformed_stmt) {
                new_statements.push_back(std::move(transformed_stmt));
            }
        }
    }
    node.statements = std::move(new_statements);
}

void ManifestResolutionPass::visit(ManifestDeclaration& node) {
    if (!current_scope_manifests_stack_.empty()) {
        current_scope_manifests_stack_.top()[node.name] = node.value;
    }
    if (current_scope_name_ == "Global") {
        global_manifests_[node.name] = node.value;
    }
    current_transformed_node_ = nullptr;
}

void ManifestResolutionPass::visit(LetDeclaration& node) {
    for (auto& init : node.initializers) {
        init = visit_expr(std::move(init));
    }
}

void ManifestResolutionPass::visit(FunctionDeclaration& node) {
    current_scope_name_ = node.name;
    current_scope_manifests_stack_.push({});
    node.body = visit_expr(std::move(node.body));
    current_scope_manifests_stack_.pop();
    current_scope_name_ = "Global";
}

void ManifestResolutionPass::visit(RoutineDeclaration& node) {
    current_scope_name_ = node.name;
    current_scope_manifests_stack_.push({});
    node.body = visit_stmt(std::move(node.body));
    current_scope_manifests_stack_.pop();
    current_scope_name_ = "Global";
}

void ManifestResolutionPass::visit(BlockStatement& node) {
    std::ostringstream block_name_ss;
    block_name_ss << current_scope_name_ << "_block_" << current_scope_manifests_stack_.size();
    std::string previous_scope_name = current_scope_name_;
    current_scope_name_ = block_name_ss.str();

    current_scope_manifests_stack_.push({});
    for (auto& decl : node.declarations) {
        decl = visit_decl(std::move(decl));
    }
    node.declarations.erase(std::remove(node.declarations.begin(), node.declarations.end(), nullptr), node.declarations.end());

    for (auto& stmt : node.statements) {
        stmt = visit_stmt(std::move(stmt));
    }
    node.statements.erase(std::remove(node.statements.begin(), node.statements.end(), nullptr), node.statements.end());

    current_scope_manifests_stack_.pop();
    current_scope_name_ = previous_scope_name;
}

void ManifestResolutionPass::visit(VariableAccess& node) {
    std::optional<int64_t> manifest_value = get_manifest_value(node.name);
    if (manifest_value.has_value()) {
        current_transformed_node_ = std::make_unique<NumberLiteral>(*manifest_value);
    }
}

void ManifestResolutionPass::visit(NumberLiteral& node) {}
void ManifestResolutionPass::visit(StringLiteral& node) {}
void ManifestResolutionPass::visit(CharLiteral& node) {}
void ManifestResolutionPass::visit(BooleanLiteral& node) {}

void ManifestResolutionPass::visit(BinaryOp& node) {
    node.left = visit_expr(std::move(node.left));
    node.right = visit_expr(std::move(node.right));
}
void ManifestResolutionPass::visit(UnaryOp& node) {
    node.operand = visit_expr(std::move(node.operand));
}
void ManifestResolutionPass::visit(VectorAccess& node) {
    node.vector_expr = visit_expr(std::move(node.vector_expr));
    node.index_expr = visit_expr(std::move(node.index_expr));
}
void ManifestResolutionPass::visit(CharIndirection& node) {
    node.string_expr = visit_expr(std::move(node.string_expr));
    node.index_expr = visit_expr(std::move(node.index_expr));
}
void ManifestResolutionPass::visit(FloatVectorIndirection& node) {
    node.vector_expr = visit_expr(std::move(node.vector_expr));
    node.index_expr = visit_expr(std::move(node.index_expr));
}
void ManifestResolutionPass::visit(FunctionCall& node) {
    node.function_expr = visit_expr(std::move(node.function_expr));
    for (auto& arg : node.arguments) {
        arg = visit_expr(std::move(arg));
    }
}
void ManifestResolutionPass::visit(ConditionalExpression& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.true_expr = visit_expr(std::move(node.true_expr));
    node.false_expr = visit_expr(std::move(node.false_expr));
}
void ManifestResolutionPass::visit(ValofExpression& node) {
    node.body = visit_stmt(std::move(node.body));
}
void ManifestResolutionPass::visit(VecAllocationExpression& node) {
    node.size_expr = visit_expr(std::move(node.size_expr));
}
void ManifestResolutionPass::visit(StringAllocationExpression& node) {
    node.size_expr = visit_expr(std::move(node.size_expr));
}
void ManifestResolutionPass::visit(TableExpression& node) {
    for (auto& expr : node.initializers) {
        expr = visit_expr(std::move(expr));
    }
}
void ManifestResolutionPass::visit(AssignmentStatement& node) {
    for (auto& lhs_expr : node.lhs) {
        lhs_expr = visit_expr(std::move(lhs_expr));
    }
    for (auto& rhs_expr : node.rhs) {
        rhs_expr = visit_expr(std::move(rhs_expr));
    }
}
void ManifestResolutionPass::visit(RoutineCallStatement& node) {
    node.routine_expr = visit_expr(std::move(node.routine_expr));
    for (auto& arg : node.arguments) {
        arg = visit_expr(std::move(arg));
    }
}
void ManifestResolutionPass::visit(IfStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.then_branch = visit_stmt(std::move(node.then_branch));
}
void ManifestResolutionPass::visit(UnlessStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.then_branch = visit_stmt(std::move(node.then_branch));
}
void ManifestResolutionPass::visit(TestStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.then_branch = visit_stmt(std::move(node.then_branch));
    node.else_branch = visit_stmt(std::move(node.else_branch));
}
void ManifestResolutionPass::visit(WhileStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.body = visit_stmt(std::move(node.body));
}
void ManifestResolutionPass::visit(UntilStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.body = visit_stmt(std::move(node.body));
}
void ManifestResolutionPass::visit(RepeatStatement& node) {
    node.body = visit_stmt(std::move(node.body));
    if (node.condition) {
        node.condition = visit_expr(std::move(node.condition));
    }
}
void ManifestResolutionPass::visit(ForStatement& node) {
    node.start_expr = visit_expr(std::move(node.start_expr));
    node.end_expr = visit_expr(std::move(node.end_expr));
    if (node.step_expr) {
        node.step_expr = visit_expr(std::move(node.step_expr));
    }
    node.body = visit_stmt(std::move(node.body));
}
void ManifestResolutionPass::visit(SwitchonStatement& node) {
    node.expression = visit_expr(std::move(node.expression));
    for (auto& case_stmt : node.cases) {
        if (case_stmt) {
            case_stmt->constant_expr = visit_expr(std::move(case_stmt->constant_expr));
            case_stmt->command = visit_stmt(std::move(case_stmt->command));
        }
    }
    if (node.default_case) {
        node.default_case->command = visit_stmt(std::move(node.default_case->command));
    }
}
void ManifestResolutionPass::visit(CaseStatement& node) { /* Handled by SwitchonStatement */ }
void ManifestResolutionPass::visit(DefaultStatement& node) { /* Handled by SwitchonStatement */ }
void ManifestResolutionPass::visit(GotoStatement& node) {
    node.label_expr = visit_expr(std::move(node.label_expr));
}
void ManifestResolutionPass::visit(ReturnStatement& node) {}
void ManifestResolutionPass::visit(ResultisStatement& node) {
    node.expression = visit_expr(std::move(node.expression));
}
void ManifestResolutionPass::visit(CompoundStatement& node) {
    for (auto& stmt : node.statements) {
        stmt = visit_stmt(std::move(stmt));
    }
}

void ManifestResolutionPass::visit(StringStatement& node) {
    node.size_expr = visit_expr(std::move(node.size_expr));
}
void ManifestResolutionPass::visit(SysCall& node) {
    node.syscall_number = visit_expr(std::move(node.syscall_number));
    for (auto& arg : node.arguments) {
        arg = visit_expr(std::move(arg));
    }
}



void ManifestResolutionPass::visit(StaticDeclaration& node) { node.initializer = visit_expr(std::move(node.initializer)); }
void ManifestResolutionPass::visit(GlobalDeclaration& node) {}
void ManifestResolutionPass::visit(LabelDeclaration& node) { node.command = visit_stmt(std::move(node.command)); }
void ManifestResolutionPass::visit(FreeStatement& node) { node.expression_ = visit_expr(std::move(node.expression_)); }
void ManifestResolutionPass::visit(LabelTargetStatement& node) {}
void ManifestResolutionPass::visit(ConditionalBranchStatement& node) {}
void ManifestResolutionPass::visit(BreakStatement& node) {}
void ManifestResolutionPass::visit(LoopStatement& node) {}
void ManifestResolutionPass::visit(EndcaseStatement& node) {}
void ManifestResolutionPass::visit(BrkStatement& node) {}
void ManifestResolutionPass::visit(FinishStatement& node) {}
void ManifestResolutionPass::visit(GlobalVariableDeclaration& node) {
    // Check if the variable is a manifest and resolve it
    for (size_t i = 0; i < node.names.size(); i++) {
        if (i < node.initializers.size() && node.initializers[i]) {
            node.initializers[i] = visit_expr(std::move(node.initializers[i]));
        }
    }
}
