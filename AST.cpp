#include "AST.h"
#include "analysis/Visitors/VariableUsageVisitor.h"

// Helper function to clone a vector of unique_ptrs
template <typename T>
std::vector<std::unique_ptr<T>> clone_vector(const std::vector<std::unique_ptr<T>>& original_vec) {
    std::vector<std::unique_ptr<T>> cloned_vec;
    cloned_vec.reserve(original_vec.size());
    for (const auto& item : original_vec) {
        if (item) {
            cloned_vec.push_back(std::unique_ptr<T>(static_cast<T*>(item->clone().release())));
        } else {
            cloned_vec.push_back(nullptr);
        }
    }
    return cloned_vec;
}

// Helper function to clone a single unique_ptr
template <typename T>
std::unique_ptr<T> clone_unique_ptr(const std::unique_ptr<T>& original_ptr) {
    if (original_ptr) {
        return std::unique_ptr<T>(static_cast<T*>(original_ptr->clone().release()));
    }
    return nullptr;
}



void SysCall::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

void VecInitializerExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

ASTNodePtr VecInitializerExpression::clone() const {
    std::vector<ExprPtr> cloned_initializers;
    for (const auto& expr : initializers) {
        if (expr) {
            cloned_initializers.push_back(std::unique_ptr<Expression>(static_cast<Expression*>(expr->clone().release())));
        } else {
            cloned_initializers.push_back(nullptr);
        }
    }
    return std::make_unique<VecInitializerExpression>(std::move(cloned_initializers));
}

ASTNodePtr SysCall::clone() const {
    // Create a new vector for the cloned arguments.
    std::vector<ExprPtr> cloned_args;
    for (const auto& arg : arguments) {
        cloned_args.push_back(std::unique_ptr<Expression>(static_cast<Expression*>(arg->clone().release())));
    }
    // Assuming SysCall has members: function_name and syscall_number
    return std::make_unique<SysCall>(
        this->function_name,
        this->syscall_number ? std::unique_ptr<Expression>(static_cast<Expression*>(this->syscall_number->clone().release())) : nullptr,
        std::move(cloned_args)
    );
}

// --- Implement accept and clone for FloatValofExpression ---
void FloatValofExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

ASTNodePtr FloatValofExpression::clone() const {
    return std::make_unique<FloatValofExpression>(
        body ? std::unique_ptr<Statement>(static_cast<Statement*>(body->clone().release())) : nullptr
    );
}

// --- Implement accept and clone for ForEachStatement ---
void ForEachStatement::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

ASTNodePtr ForEachStatement::clone() const {
    return std::make_unique<ForEachStatement>(
        loop_variable_name,
        type_variable_name,
        collection_expression ? std::unique_ptr<Expression>(static_cast<Expression*>(collection_expression->clone().release())) : nullptr,
        body ? std::unique_ptr<Statement>(static_cast<Statement*>(body->clone().release())) : nullptr,
        filter_type
    );
}

// Implement accept methods for each AST node
// These will simply call the corresponding visit method on the visitor
#define ACCEPT_METHOD_IMPL(ClassName) \
    void ClassName::accept(ASTVisitor& visitor) { visitor.visit(*this); }

ACCEPT_METHOD_IMPL(Program)

// Declarations
ACCEPT_METHOD_IMPL(LetDeclaration)
ACCEPT_METHOD_IMPL(ManifestDeclaration)
ACCEPT_METHOD_IMPL(StaticDeclaration)
ACCEPT_METHOD_IMPL(GlobalDeclaration)
ACCEPT_METHOD_IMPL(GlobalVariableDeclaration)
ACCEPT_METHOD_IMPL(FunctionDeclaration)
ACCEPT_METHOD_IMPL(RoutineDeclaration)
ACCEPT_METHOD_IMPL(LabelDeclaration)


// Expressions
ACCEPT_METHOD_IMPL(NumberLiteral)
ACCEPT_METHOD_IMPL(StringLiteral)
ACCEPT_METHOD_IMPL(CharLiteral)
ACCEPT_METHOD_IMPL(BooleanLiteral)
ACCEPT_METHOD_IMPL(VariableAccess)
ACCEPT_METHOD_IMPL(BinaryOp)
ACCEPT_METHOD_IMPL(UnaryOp)
ACCEPT_METHOD_IMPL(VectorAccess)
ACCEPT_METHOD_IMPL(CharIndirection)
ACCEPT_METHOD_IMPL(FloatVectorIndirection)
ACCEPT_METHOD_IMPL(FunctionCall)
ACCEPT_METHOD_IMPL(BitfieldAccessExpression)
ACCEPT_METHOD_IMPL(ConditionalExpression)
ACCEPT_METHOD_IMPL(ValofExpression)

// Statements
ACCEPT_METHOD_IMPL(ConditionalBranchStatement)
ACCEPT_METHOD_IMPL(AssignmentStatement)
ACCEPT_METHOD_IMPL(RoutineCallStatement)
ACCEPT_METHOD_IMPL(IfStatement)
ACCEPT_METHOD_IMPL(UnlessStatement)
ACCEPT_METHOD_IMPL(TestStatement)
ACCEPT_METHOD_IMPL(WhileStatement)
ACCEPT_METHOD_IMPL(UntilStatement)
ACCEPT_METHOD_IMPL(RepeatStatement)
ACCEPT_METHOD_IMPL(ForStatement)
ACCEPT_METHOD_IMPL(SwitchonStatement)
ACCEPT_METHOD_IMPL(CaseStatement)
ACCEPT_METHOD_IMPL(DefaultStatement)
ACCEPT_METHOD_IMPL(GotoStatement)
ACCEPT_METHOD_IMPL(ReturnStatement)
ACCEPT_METHOD_IMPL(FinishStatement)
ACCEPT_METHOD_IMPL(BreakStatement)
ACCEPT_METHOD_IMPL(LoopStatement)
ACCEPT_METHOD_IMPL(EndcaseStatement)
ACCEPT_METHOD_IMPL(ResultisStatement)
ACCEPT_METHOD_IMPL(CompoundStatement)
ACCEPT_METHOD_IMPL(BlockStatement)
ACCEPT_METHOD_IMPL(StringStatement)
ACCEPT_METHOD_IMPL(VecAllocationExpression)
ACCEPT_METHOD_IMPL(FVecAllocationExpression)
ACCEPT_METHOD_IMPL(StringAllocationExpression)
ACCEPT_METHOD_IMPL(TableExpression)

ASTNodePtr FVecAllocationExpression::clone() const {
    // Deep clone the size expression and copy the variable name
    return std::make_unique<FVecAllocationExpression>(
        size_expr ? std::unique_ptr<Expression>(static_cast<Expression*>(size_expr->clone().release())) : nullptr
    );
}

ACCEPT_METHOD_IMPL(LabelTargetStatement)


ACCEPT_METHOD_IMPL(BrkStatement)
ACCEPT_METHOD_IMPL(FreeStatement)

ASTNodePtr BrkStatement::clone() const {
    return std::make_unique<BrkStatement>();
}

#undef ACCEPT_METHOD_IMPL

// --- Implement accept and clone for ListExpression ---
void ListExpression::accept(ASTVisitor& visitor) {
    visitor.visit(*this);
}

ASTNodePtr ListExpression::clone() const {
    std::vector<ExprPtr> cloned_initializers;
    for (const auto& expr : initializers) {
        cloned_initializers.push_back(expr ? std::unique_ptr<Expression>(static_cast<Expression*>(expr->clone().release())) : nullptr);
    }
    auto node = std::make_unique<ListExpression>(std::move(cloned_initializers), is_manifest);
    return node;
}
#undef ACCEPT_METHOD_IMPL

// --- Implement accept and clone for FreeListStatement ---


// --- Implement clone for GlobalVariableDeclaration --- //
ASTNodePtr GlobalVariableDeclaration::clone() const {
    std::vector<ExprPtr> cloned_initializers;
    for (const auto& init : initializers) {
        cloned_initializers.push_back(init ? std::unique_ptr<Expression>(static_cast<Expression*>(init->clone().release())) : nullptr);
    }
    auto cloned = std::make_unique<GlobalVariableDeclaration>(names, std::move(cloned_initializers));
    cloned->is_float_declaration = is_float_declaration;
    return cloned;
}


ASTNodePtr CaseStatement::clone() const {
    // Deep clone constant_expr and command, and copy resolved_constant_value
    auto cloned_constant_expr = constant_expr ? std::unique_ptr<Expression>(static_cast<Expression*>(constant_expr->clone().release())) : nullptr;
    auto cloned_command = command ? std::unique_ptr<Statement>(static_cast<Statement*>(command->clone().release())) : nullptr;
    auto cloned_case = std::make_unique<CaseStatement>(std::move(cloned_constant_expr), std::move(cloned_command));
    cloned_case->resolved_constant_value = resolved_constant_value;
    return cloned_case;
}
