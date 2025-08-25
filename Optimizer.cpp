#include "Optimizer.h"
#include "AST.h"
#include "ASTVisitor.h"
#include <stdexcept>
#include <algorithm> // For std::remove

Optimizer::Optimizer(std::unordered_map<std::string, int64_t>& manifests)
    : manifests_(manifests) {}

ProgramPtr Optimizer::apply(ProgramPtr program) {
    // Default implementation: traverse and allow node replacement via current_transformed_node_
    current_transformed_node_ = std::move(program);
    current_transformed_node_->accept(*this);
    return std::unique_ptr<Program>(static_cast<Program*>(current_transformed_node_.release()));
}

// Helper methods for visiting and potentially replacing nodes
ExprPtr Optimizer::visit_expr(ExprPtr expr) {
    if (!expr) return nullptr;
    ASTNodePtr parent_context = std::move(current_transformed_node_); // Save parent context
    current_transformed_node_ = std::move(expr);
    current_transformed_node_->accept(*this);
    ExprPtr result = ExprPtr(static_cast<Expression*>(current_transformed_node_.release()));
    current_transformed_node_ = std::move(parent_context); // Restore parent context
    return result;
}

StmtPtr Optimizer::visit_stmt(StmtPtr stmt) {
    if (!stmt) return nullptr;
    ASTNodePtr parent_context = std::move(current_transformed_node_); // Save parent context
    current_transformed_node_ = std::move(stmt);
    current_transformed_node_->accept(*this);
    StmtPtr result = StmtPtr(static_cast<Statement*>(current_transformed_node_.release()));
    current_transformed_node_ = std::move(parent_context); // Restore parent context
    return result;
}

DeclPtr Optimizer::visit_decl(DeclPtr decl) {
    if (!decl) return nullptr;
    ASTNodePtr parent_context = std::move(current_transformed_node_); // Save parent context
    current_transformed_node_ = std::move(decl);
    current_transformed_node_->accept(*this);
    DeclPtr result = DeclPtr(static_cast<Declaration*>(current_transformed_node_.release()));
    current_transformed_node_ = std::move(parent_context); // Restore parent context
    return result;
}

// --- Default ASTVisitor implementations (traverse children) ---
// These methods simply call accept on their children, allowing for recursive traversal.
// Derived classes will override these to implement specific optimization logic.

void Optimizer::visit(Program& node) {
    // Create safe copies of the declarations and statements vectors
    std::vector<DeclPtr> processed_declarations;
    processed_declarations.reserve(node.declarations.size());
    
    std::vector<StmtPtr> processed_statements;
    processed_statements.reserve(node.statements.size());

    // Process each declaration individually
    for (auto& decl : node.declarations) {
        if (decl) {
            processed_declarations.push_back(visit_decl(std::move(decl)));
        }
    }
    
    // Process each statement individually
    for (auto& stmt : node.statements) {
        if (stmt) {
            processed_statements.push_back(visit_stmt(std::move(stmt)));
        }
    }

    // Clear original vectors and swap with our safely processed ones
    node.declarations.clear();
    node.declarations = std::move(processed_declarations);
    
    node.statements.clear();
    node.statements = std::move(processed_statements);
    // After traversal, current_transformed_node_ still holds the original Program node
}

// Declarations
void Optimizer::visit(LetDeclaration& node) {
    for (auto& init : node.initializers) {
        init = visit_expr(std::move(init));
    }
    // current_transformed_node_ remains unchanged unless overridden
}

void Optimizer::visit(ManifestDeclaration& node) {
    // Manifests are typically constant, no children to visit.
    // current_transformed_node_ remains unchanged
}

void Optimizer::visit(StaticDeclaration& node) {
    node.initializer = visit_expr(std::move(node.initializer));
    // current_transformed_node_ remains unchanged
}

void Optimizer::visit(GlobalDeclaration& node) {
    // Globals are typically constant offsets, no children to visit.
    // current_transformed_node_ remains unchanged
}

void Optimizer::visit(FunctionDeclaration& node) {
    node.body = visit_expr(std::move(node.body));
    // current_transformed_node_ remains unchanged
}



void Optimizer::visit(SysCall& node) {
    node.syscall_number = visit_expr(std::move(node.syscall_number));
    for (auto& arg : node.arguments) {
        arg = visit_expr(std::move(arg));
    }
}

void Optimizer::visit(RoutineDeclaration& node) {
    // Default behavior for the optimizer: recursively visit the body of the routine.
    // The body of a RoutineDeclaration is a Statement.
    node.body = visit_stmt(std::move(node.body));
    // current_transformed_node_ remains unchanged unless overridden by derived class
}

void Optimizer::visit(LabelDeclaration& node) {
    node.command = visit_stmt(std::move(node.command));
}



// Expressions
void Optimizer::visit(NumberLiteral& node) {}
void Optimizer::visit(StringLiteral& node) {}
void Optimizer::visit(CharLiteral& node) {}
void Optimizer::visit(BooleanLiteral& node) {}

void Optimizer::visit(VariableAccess& node) {
    // No children to visit.
}

void Optimizer::visit(BinaryOp& node) {
    node.left = visit_expr(std::move(node.left));
    node.right = visit_expr(std::move(node.right));
}

void Optimizer::visit(UnaryOp& node) {
    node.operand = visit_expr(std::move(node.operand));
}

void Optimizer::visit(VectorAccess& node) {
    node.vector_expr = visit_expr(std::move(node.vector_expr));
    node.index_expr = visit_expr(std::move(node.index_expr));
}

void Optimizer::visit(CharIndirection& node) {
    node.string_expr = visit_expr(std::move(node.string_expr));
    node.index_expr = visit_expr(std::move(node.index_expr));
}

void Optimizer::visit(FloatVectorIndirection& node) {
    node.vector_expr = visit_expr(std::move(node.vector_expr));
    node.index_expr = visit_expr(std::move(node.index_expr));
}

void Optimizer::visit(FunctionCall& node) {
    node.function_expr = visit_expr(std::move(node.function_expr));
    for (auto& arg : node.arguments) {
        arg = visit_expr(std::move(arg));
    }
}

void Optimizer::visit(ConditionalExpression& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.true_expr = visit_expr(std::move(node.true_expr));
    node.false_expr = visit_expr(std::move(node.false_expr));
}

void Optimizer::visit(ValofExpression& node) {
    node.body = visit_stmt(std::move(node.body));
}

// Statements
void Optimizer::visit(AssignmentStatement& node) {
    for (auto& lhs_expr : node.lhs) {
        lhs_expr = visit_expr(std::move(lhs_expr));
    }
    for (auto& rhs_expr : node.rhs) {
        rhs_expr = visit_expr(std::move(rhs_expr));
    }
}

void Optimizer::visit(RoutineCallStatement& node) {
    node.routine_expr = visit_expr(std::move(node.routine_expr));
    for (auto& arg : node.arguments) {
        arg = visit_expr(std::move(arg));
    }
}

void Optimizer::visit(IfStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.then_branch = visit_stmt(std::move(node.then_branch));
}

void Optimizer::visit(UnlessStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.then_branch = visit_stmt(std::move(node.then_branch));
}

void Optimizer::visit(TestStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.then_branch = visit_stmt(std::move(node.then_branch));
    node.else_branch = visit_stmt(std::move(node.else_branch));
}

void Optimizer::visit(WhileStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.body = visit_stmt(std::move(node.body));
}

void Optimizer::visit(UntilStatement& node) {
    node.condition = visit_expr(std::move(node.condition));
    node.body = visit_stmt(std::move(node.body));
}

void Optimizer::visit(RepeatStatement& node) {
    node.body = visit_stmt(std::move(node.body));
    if (node.condition) {
        node.condition = visit_expr(std::move(node.condition));
    }
}

void Optimizer::visit(ForStatement& node) {
    node.start_expr = visit_expr(std::move(node.start_expr));
    node.end_expr = visit_expr(std::move(node.end_expr));
    if (node.step_expr) {
        node.step_expr = visit_expr(std::move(node.step_expr));
    }
    node.body = visit_stmt(std::move(node.body));
}

void Optimizer::visit(SwitchonStatement& node) {
    // Process the expression safely
    if (node.expression) {
        node.expression = visit_expr(std::move(node.expression));
    }
    
    // Process each case safely
    std::vector<std::unique_ptr<CaseStatement>> processed_cases;
    processed_cases.reserve(node.cases.size());
    
    for (auto& case_stmt : node.cases) {
        if (case_stmt) {
            auto new_case = std::make_unique<CaseStatement>(nullptr, nullptr);
            
            // Process the constant expression
            if (case_stmt->constant_expr) {
                new_case->constant_expr = visit_expr(std::move(case_stmt->constant_expr));
            }
            
            // Process the command
            if (case_stmt->command) {
                new_case->command = visit_stmt(std::move(case_stmt->command));
            }
            
            // Store the resolved constant value if it exists
            if (case_stmt->resolved_constant_value.has_value()) {
                new_case->resolved_constant_value = case_stmt->resolved_constant_value;
            }
            
            processed_cases.push_back(std::move(new_case));
        }
    }
    
    // Replace the cases with our safely processed ones
    node.cases.clear();
    node.cases = std::move(processed_cases);
    
    // Process the default case if it exists
    if (node.default_case && node.default_case->command) {
        node.default_case->command = visit_stmt(std::move(node.default_case->command));
    }
}

void Optimizer::visit(CaseStatement& node) {
    // Handled by SwitchonStatement
}

void Optimizer::visit(DefaultStatement& node) {
    // Handled by SwitchonStatement
}

void Optimizer::visit(GotoStatement& node) {
    node.label_expr = visit_expr(std::move(node.label_expr));
}

void Optimizer::visit(ReturnStatement& node) {
    // No children to visit.
}

void Optimizer::visit(FinishStatement& node) {
    // No children to visit.
}

void Optimizer::visit(BreakStatement& node) {
    // No children to visit.
}

void Optimizer::visit(LoopStatement& node) {
    // No children to visit.
}

void Optimizer::visit(EndcaseStatement& node) {
    // No children to visit.
}

void Optimizer::visit(FreeStatement& node) {
    node.list_expr = visit_expr(std::move(node.list_expr));
}

void Optimizer::visit(CompoundStatement& node) {
    // Create a safe copy of the statements vector
    std::vector<StmtPtr> processed_statements;
    processed_statements.reserve(node.statements.size());

    // Process each statement individually
    for (auto& stmt : node.statements) {
        if (stmt) {
            processed_statements.push_back(visit_stmt(std::move(stmt)));
        }
    }

    // Clear original vector and swap with our safely processed statements
    node.statements.clear();
    node.statements = std::move(processed_statements);
}

void Optimizer::visit(BlockStatement& node) {
    // Create a safe copy of the statements vector
    std::vector<StmtPtr> processed_statements;
    processed_statements.reserve(node.statements.size());

    // Process each statement individually
    for (auto& stmt : node.statements) {
        if (stmt) {
            processed_statements.push_back(visit_stmt(std::move(stmt)));
        }
    }

    // Clear original vector and swap with our safely processed statements
    node.statements.clear();
    node.statements = std::move(processed_statements);
}

void Optimizer::visit(StringStatement& node) {
    node.size_expr = visit_expr(std::move(node.size_expr));
}

void Optimizer::visit(BrkStatement& node) {
    // No children to visit.
}

void Optimizer::visit(LabelTargetStatement& node) {
    // No children to visit.
}

void Optimizer::visit(ConditionalBranchStatement& node) {
    // No children to visit.
    }

    void Optimizer::visit(ResultisStatement& node) {
        if (node.expression) node.expression->accept(*this);
    }



void Optimizer::visit(VecAllocationExpression& node) {
    node.size_expr = visit_expr(std::move(node.size_expr));
}

void Optimizer::visit(StringAllocationExpression& node) {
    node.size_expr = visit_expr(std::move(node.size_expr));
}

void Optimizer::visit(TableExpression& node) {
    for (auto& expr : node.initializers) {
        expr = visit_expr(std::move(expr));
    }
}
