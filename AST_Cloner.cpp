#include "AST.h" // - Required for AST node definitions and smart pointer typedefs
#include "ASTVisitor.h"

// Helper function to clone a vector of unique_ptrs
template <typename T>
std::vector<std::unique_ptr<T>> clone_vector(const std::vector<std::unique_ptr<T>>& original_vec) {
    std::vector<std::unique_ptr<T>> cloned_vec; //
    cloned_vec.reserve(original_vec.size()); //
    for (const auto& item : original_vec) { //
        if (item) { //
            // item->clone() returns an ASTNodePtr (unique_ptr<ASTNode>).
            // We need to static_cast the raw pointer obtained from .release() back to T*.
            // This cast is safe because we know the original vector holds unique_ptr<T>,
            // and clone() returns a dynamically allocated object of the correct derived type.
            cloned_vec.push_back(std::unique_ptr<T>(static_cast<T*>(item->clone().release()))); //
        } else {
            cloned_vec.push_back(nullptr); // Preserve nullptr entries to maintain vector structure
        }
    }
    return cloned_vec; //
}

// Helper function to clone a single unique_ptr
template <typename T>
std::unique_ptr<T> clone_unique_ptr(const std::unique_ptr<T>& original_ptr) {
    if (original_ptr) { //
        // Similar to the vector helper, cast the raw pointer to T*
        return std::unique_ptr<T>(static_cast<T*>(original_ptr->clone().release())); //
    }
    return nullptr; //
}

// --- Program --- //
ASTNodePtr Program::clone() const { //
    auto cloned_program = std::make_unique<Program>(); //
    cloned_program->declarations = clone_vector(declarations); //
    cloned_program->statements = clone_vector(statements); //
    return cloned_program; //
}



// --- Declarations --- //
ASTNodePtr LetDeclaration::clone() const { //
    // Uses the generic clone_vector for ExprPtr
    return std::make_unique<LetDeclaration>(names, clone_vector(initializers)); //
}

ASTNodePtr ManifestDeclaration::clone() const { //
    return std::make_unique<ManifestDeclaration>(name, value); //
}

ASTNodePtr StaticDeclaration::clone() const { //
    auto cloned = std::make_unique<StaticDeclaration>(name, clone_unique_ptr(initializer)); //
    cloned->is_float_declaration = this->is_float_declaration; //
    return cloned; //
}

ASTNodePtr GlobalDeclaration::clone() const { //
    return std::make_unique<GlobalDeclaration>(globals); //
}

ASTNodePtr FunctionDeclaration::clone() const { //
    return std::make_unique<FunctionDeclaration>(name, parameters, clone_unique_ptr(body)); //
}

ASTNodePtr RoutineDeclaration::clone() const { //
    return std::make_unique<RoutineDeclaration>(name, parameters, clone_unique_ptr(body)); //
}

ASTNodePtr LabelDeclaration::clone() const { //
    return std::make_unique<LabelDeclaration>(name, clone_unique_ptr(command)); //
}



// --- Expressions --- //
ASTNodePtr NumberLiteral::clone() const { //
    if (literal_type == LiteralType::Integer) { //
        return std::make_unique<NumberLiteral>(int_value); //
    } else {
        return std::make_unique<NumberLiteral>(float_value); //
    }
}

ASTNodePtr StringLiteral::clone() const { //
    return std::make_unique<StringLiteral>(value); //
}

ASTNodePtr CharLiteral::clone() const { //
    return std::make_unique<CharLiteral>(value); //
}

ASTNodePtr BooleanLiteral::clone() const { //
    return std::make_unique<BooleanLiteral>(value); //
}

ASTNodePtr VariableAccess::clone() const { //
    return std::make_unique<VariableAccess>(name); //
}

ASTNodePtr BinaryOp::clone() const { //
    return std::make_unique<BinaryOp>(op, clone_unique_ptr(left), clone_unique_ptr(right)); //
}

ASTNodePtr UnaryOp::clone() const { //
    return std::make_unique<UnaryOp>(op, clone_unique_ptr(operand)); //
}

ASTNodePtr VectorAccess::clone() const { //
    return std::make_unique<VectorAccess>(clone_unique_ptr(vector_expr), clone_unique_ptr(index_expr)); //
}

ASTNodePtr CharIndirection::clone() const { //
    return std::make_unique<CharIndirection>(clone_unique_ptr(string_expr), clone_unique_ptr(index_expr)); //
}

ASTNodePtr FloatVectorIndirection::clone() const { //
    return std::make_unique<FloatVectorIndirection>(clone_unique_ptr(vector_expr), clone_unique_ptr(index_expr)); //
}

ASTNodePtr BitfieldAccessExpression::clone() const {
    return std::make_unique<BitfieldAccessExpression>(
        clone_unique_ptr(base_expr),
        clone_unique_ptr(start_bit_expr),
        clone_unique_ptr(width_expr)
    );
}

ASTNodePtr FunctionCall::clone() const { //
    // Uses the generic clone_vector for ExprPtr
    return std::make_unique<FunctionCall>(clone_unique_ptr(function_expr), clone_vector(arguments)); //
}

ASTNodePtr ConditionalExpression::clone() const { //
    return std::make_unique<ConditionalExpression>(clone_unique_ptr(condition), clone_unique_ptr(true_expr), clone_unique_ptr(false_expr)); //
}

ASTNodePtr ValofExpression::clone() const { //
    return std::make_unique<ValofExpression>(clone_unique_ptr(body)); //
}

ASTNodePtr VecAllocationExpression::clone() const { //
    return std::make_unique<VecAllocationExpression>(clone_unique_ptr(size_expr)); //
}

ASTNodePtr StringAllocationExpression::clone() const { //
    return std::make_unique<StringAllocationExpression>(clone_unique_ptr(size_expr)); //
}

ASTNodePtr FreeStatement::clone() const { //
    return std::make_unique<FreeStatement>(clone_unique_ptr(list_expr)); //
}

ASTNodePtr TableExpression::clone() const { //
    auto cloned = std::make_unique<TableExpression>(clone_vector(initializers), is_float_table); //
    return cloned; //
}

// --- Statements --- //
ASTNodePtr AssignmentStatement::clone() const { //
    // Uses generic clone_vector for ExprPtrs
    return std::make_unique<AssignmentStatement>(clone_vector(lhs), clone_vector(rhs)); //
}

ASTNodePtr RoutineCallStatement::clone() const { //
    // Uses generic clone_vector for ExprPtr
    return std::make_unique<RoutineCallStatement>(clone_unique_ptr(routine_expr), clone_vector(arguments)); //
}

ASTNodePtr IfStatement::clone() const { //
    return std::make_unique<IfStatement>(clone_unique_ptr(condition), clone_unique_ptr(then_branch)); //
}

ASTNodePtr UnlessStatement::clone() const { //
    return std::make_unique<UnlessStatement>(clone_unique_ptr(condition), clone_unique_ptr(then_branch)); //
}

ASTNodePtr TestStatement::clone() const { //
    return std::make_unique<TestStatement>(clone_unique_ptr(condition), clone_unique_ptr(then_branch), clone_unique_ptr(else_branch)); //
}

ASTNodePtr WhileStatement::clone() const { //
    return std::make_unique<WhileStatement>(clone_unique_ptr(condition), clone_unique_ptr(body)); //
}

ASTNodePtr UntilStatement::clone() const { //
    return std::make_unique<UntilStatement>(clone_unique_ptr(condition), clone_unique_ptr(body)); //
}

ASTNodePtr RepeatStatement::clone() const { //
    return std::make_unique<RepeatStatement>(loop_type, clone_unique_ptr(body), clone_unique_ptr(condition)); //
}

ASTNodePtr ForStatement::clone() const { //
    auto cloned = std::make_unique<ForStatement>(loop_variable, clone_unique_ptr(start_expr), clone_unique_ptr(end_expr), clone_unique_ptr(body), clone_unique_ptr(step_expr)); //
    cloned->unique_loop_variable_name = unique_loop_variable_name;
    cloned->unique_step_variable_name = unique_step_variable_name;
    cloned->unique_end_variable_name = unique_end_variable_name;
    return cloned;
}

// CaseStatement and DefaultStatement are ASTNode, not Statement, but their clone methods return ASTNodePtr

ASTNodePtr LabelTargetStatement::clone() const {
    return std::make_unique<LabelTargetStatement>(labelName);
}

ASTNodePtr ConditionalBranchStatement::clone() const {
    // Clone the condition_expr if it exists, otherwise pass nullptr.
    // Clone the condition_expr if it exists, otherwise pass nullptr.
    ExprPtr cond_expr_clone = condition_expr ? std::unique_ptr<Expression>(static_cast<Expression*>(condition_expr->clone().release())) : nullptr;
    return std::make_unique<ConditionalBranchStatement>(condition, targetLabel, std::move(cond_expr_clone));
}

ASTNodePtr DefaultStatement::clone() const { //
    return std::make_unique<DefaultStatement>(clone_unique_ptr(command)); //
}

ASTNodePtr SwitchonStatement::clone() const { //
    // Manually clone the vector of unique_ptr<CaseStatement>
    std::vector<std::unique_ptr<CaseStatement>> cloned_cases; //
    cloned_cases.reserve(cases.size()); //
    for (const auto& case_stmt : cases) { //
        if (case_stmt) { //
            // case_stmt->clone() returns ASTNodePtr. We static_cast it to CaseStatement* before making a unique_ptr<CaseStatement>.
            cloned_cases.push_back(std::unique_ptr<CaseStatement>(static_cast<CaseStatement*>(case_stmt->clone().release()))); //
        } else {
            cloned_cases.push_back(nullptr); // Preserve nullptr entries
        }
    }

    // clone_unique_ptr handles unique_ptr<DefaultStatement> directly
    std::unique_ptr<DefaultStatement> cloned_default_case = clone_unique_ptr(default_case); //

    return std::make_unique<SwitchonStatement>(
        clone_unique_ptr(expression),
        std::move(cloned_cases),
        std::move(cloned_default_case)
    ); //
}

ASTNodePtr GotoStatement::clone() const { //
    return std::make_unique<GotoStatement>(clone_unique_ptr(label_expr)); //
}

ASTNodePtr ReturnStatement::clone() const { //
    return std::make_unique<ReturnStatement>(); //
}

ASTNodePtr FinishStatement::clone() const { //
    return std::make_unique<FinishStatement>(); //
}

ASTNodePtr BreakStatement::clone() const { //
    return std::make_unique<BreakStatement>(); //
}

ASTNodePtr LoopStatement::clone() const { //
    return std::make_unique<LoopStatement>(); //
}

ASTNodePtr EndcaseStatement::clone() const { //
    return std::make_unique<EndcaseStatement>(); //
}

ASTNodePtr ResultisStatement::clone() const { //
    return std::make_unique<ResultisStatement>(clone_unique_ptr(expression)); //
}

ASTNodePtr CompoundStatement::clone() const { //
    return std::make_unique<CompoundStatement>(clone_vector(statements)); //
}

ASTNodePtr BlockStatement::clone() const { //
    // Uses generic clone_vector for StmtPtr only (LetDeclaration is now a Statement)
    return std::make_unique<BlockStatement>(std::vector<DeclPtr>{}, clone_vector(statements)); //
}

ASTNodePtr StringStatement::clone() const { //
    return std::make_unique<StringStatement>(clone_unique_ptr(size_expr)); //
}

// ASTNodePtr BrkStatement::clone() const { //
//     return std::make_unique<BrkStatement>(); //
// }
