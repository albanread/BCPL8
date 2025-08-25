#include "Parser.h"
#include <stdexcept>
#include <vector>
#include "AST.h"

/**
 * @brief Parses a statement, handling optional REPEAT WHILE/UNTIL clauses.
 * This is the main entry point for parsing any statement.
 */
StmtPtr Parser::parse_statement() {
    if (fatal_error_) return nullptr;
    TraceGuard guard(*this, "parse_statement");

    // First, parse the primary statement part.
    auto statement = parse_primary_statement();
    if (fatal_error_) return nullptr;

    // After a statement, there might be a REPEAT clause.
    if (match(TokenType::Repeat)) {
        if (match(TokenType::While)) {
            auto condition = parse_expression();
            return std::make_unique<RepeatStatement>(RepeatStatement::LoopType::RepeatWhile, std::move(statement), std::move(condition));
        }
        if (match(TokenType::Until)) {
            auto condition = parse_expression();
            return std::make_unique<RepeatStatement>(RepeatStatement::LoopType::RepeatUntil, std::move(statement), std::move(condition));
        }
        return std::make_unique<RepeatStatement>(RepeatStatement::LoopType::Repeat, std::move(statement));
    }

    return statement;
}

/**
 * @brief Parses a non-REPEAT statement based on the current token.
 */
StmtPtr Parser::parse_primary_statement() {
    TraceGuard guard(*this, "parse_primary_statement");

    // Check for a label target as the start of a statement.
    if (check(TokenType::Identifier) && lexer_.peek().type == TokenType::Colon) {
        std::string label_name = current_token_.value;
        consume(TokenType::Identifier, "Expect identifier for label name.");
        consume(TokenType::Colon, "Expect ':' after label name.");
        return std::make_unique<LabelTargetStatement>(label_name);
    }
    
    // A statement starting with a brace is a block or compound statement.
    if (check(TokenType::LBrace)) {
        return parse_block_or_compound_statement();
    }

    // Handle all other statement types based on their keyword.
    switch (current_token_.type) {
        case TokenType::Repeat:     return parse_repeat_statement(); // <-- ADDED
        case TokenType::If:         return parse_if_statement();
        case TokenType::Unless:     return parse_unless_statement();
        case TokenType::Test:       return parse_test_statement();
        case TokenType::While:      return parse_while_statement();
        case TokenType::For:        return parse_for_statement();
        case TokenType::ForEach:    return parse_foreach_statement();
        case TokenType::Switchon:   return parse_switchon_statement();
        case TokenType::Goto:       return parse_goto_statement();
        case TokenType::FREEVEC:    return parse_free_statement();
        case TokenType::Return:     return parse_return_statement();
        case TokenType::Finish:     return parse_finish_statement();
        case TokenType::Break:      return parse_break_statement();
        case TokenType::Brk:        return parse_brk_statement();
        case TokenType::Loop:       return parse_loop_statement();
        case TokenType::Endcase:    return parse_endcase_statement();
        case TokenType::Resultis:   return parse_resultis_statement();
        
        default:
            // If it's not a keyword-led statement, it must be
            // an assignment or routine call, which starts with an expression.
            return parse_assignment_or_routine_call();
    }
}

/**
 * @brief Parses a FOREACH statement.
 */
StmtPtr Parser::parse_foreach_statement() {
    TraceGuard guard(*this, "parse_foreach_statement");
    advance(); // Consume FOREACH

    // --- REVISED LOGIC ---
    std::string loop_var_name, type_var_name;

    // Read the first identifier. It could be the loop variable `e` or the type variable `T`.
    std::string first_var = current_token_.value;
    consume(TokenType::Identifier, "Expect at least one variable name after FOREACH.");

    // Look ahead to see if there's a comma, which indicates the two-variable form.
    if (match(TokenType::Comma)) {
        // This is the "FOREACH T, V" form.
        type_var_name = first_var; // The first variable was the type.
        loop_var_name = current_token_.value; // The second variable is the value.
        consume(TokenType::Identifier, "Expect value variable name after comma.");
    } else {
        // This is the single-variable "FOREACH e" form.
        loop_var_name = first_var; // The first variable was the value.
        // type_var_name remains empty, which is the correct state for this form.
    }
    // --- END REVISED LOGIC ---

    consume(TokenType::In, "Expect IN after loop variable(s).");
    auto collection_expr = parse_expression();

    // A filter type is no longer parsed here but is inferred by the analyzer.
    // The VarType::ANY argument is a placeholder.

    consume(TokenType::Do, "Expect DO after collection expression.");
    auto body = parse_statement();

    return std::make_unique<ForEachStatement>(loop_var_name, type_var_name, std::move(collection_expr), std::move(body), VarType::ANY);
}

// --- REPEAT statement parsing ---

StmtPtr Parser::parse_repeat_statement() {
    TraceGuard guard(*this, "parse_repeat_statement");
    consume(TokenType::Repeat, "Expect 'REPEAT'.");

    // Now parse the loop body, which is a statement.
    auto body = parse_statement();

    // After the body, expect the 'UNTIL' keyword.
    consume(TokenType::Until, "Expect 'UNTIL' after REPEAT body.");

    // Finally, parse the condition expression.
    auto condition = parse_expression();

    // Create the AST node.
    return std::make_unique<RepeatStatement>(
        RepeatStatement::LoopType::RepeatUntil,
        std::move(body),
        std::move(condition)
    );
}

/**
 * @brief Parses an assignment (e.g., x := 1) or a routine call (e.g., F(y)).
 */
StmtPtr Parser::parse_assignment_or_routine_call() {
    TraceGuard guard(*this, "parse_assignment_or_routine_call");

    // First, parse it as a generic expression.
    ExprPtr expr = parse_expression();
    if (!expr) {
        error("Expected an expression for assignment or routine call.");
        return nullptr;
    }


    // If the parsed expression is a FunctionCall node, it's a routine call.
    if (expr->getType() == ASTNode::NodeType::FunctionCallExpr) {
        auto call_expr = static_cast<FunctionCall*>(expr.release());
        return std::make_unique<RoutineCallStatement>(std::move(call_expr->function_expr), std::move(call_expr->arguments));
    }

    // Otherwise, it must be an assignment.
    std::vector<ExprPtr> lhs_exprs;
    lhs_exprs.push_back(std::move(expr));

    // Handle multiple assignment targets (e.g., x, y, z := ...).
    while (match(TokenType::Comma)) {
        lhs_exprs.push_back(parse_expression());
    }

    consume(TokenType::Assign, "Expect ':=' for an assignment statement.");

    // Handle multiple assignment values (e.g., ... := 1, 2, 3).
    std::vector<ExprPtr> rhs_exprs;
    do {
        rhs_exprs.push_back(parse_expression());
    } while (match(TokenType::Comma));

    if (lhs_exprs.size() != rhs_exprs.size()) {
        error("Mismatch in number of l-values and r-values for assignment.");
        return nullptr;
    }
    return std::make_unique<AssignmentStatement>(std::move(lhs_exprs), std::move(rhs_exprs));
}

/**
 * @brief Parses a block with declarations `$(...)$` or a compound statement `{...}`.
 */
StmtPtr Parser::parse_block_or_compound_statement() {
    TraceGuard guard(*this, "parse_block_or_compound_statement");
    consume(TokenType::LBrace, "Expect '$(' or '{' to start a block.");

    std::vector<DeclPtr> declarations;
    std::vector<StmtPtr> statements;

    while (!check(TokenType::RBrace) && !is_at_end()) {
        if (check(TokenType::Let) || check(TokenType::FLet)) {
            // Parse LET/FLET as a statement and add to the list.
            statements.push_back(parse_let_statement());
        } else if (is_declaration_start()) {
            declarations.push_back(parse_declaration());
        } else {
            auto stmt = parse_statement();
            if (!stmt) break; // Stop on error
            statements.push_back(std::move(stmt));
        }
    }

    consume(TokenType::RBrace, "Expect '$)' or '}' to end a block.");

    // If there were no declarations, it's a simple CompoundStatement.
    if (declarations.empty()) {
        return std::make_unique<CompoundStatement>(std::move(statements));
    }
    return std::make_unique<BlockStatement>(std::move(declarations), std::move(statements));
}

/**
 * @brief Parses a new LET or FLET statement inside a block.
 * Inside a block, LET can only declare local variables, so it always produces a Statement.
 */
StmtPtr Parser::parse_let_statement() {
    TraceGuard guard(*this, "parse_let_statement");

    bool is_float = check(TokenType::FLet);
    consume(is_float ? TokenType::FLet : TokenType::Let, "Expect 'LET' or 'FLET'.");

    std::vector<std::string> names;
    do {
        names.push_back(current_token_.value);
        consume(TokenType::Identifier, "Expect identifier in LET declaration.");
    } while (match(TokenType::Comma));

    consume(TokenType::Equal, "Expect '=' after name(s) in LET declaration.");

    std::vector<ExprPtr> initializers;
    do {
        initializers.push_back(parse_expression());
    } while (match(TokenType::Comma));

    if (names.size() != initializers.size()) {
        error("Mismatch between number of names and initializers in LET declaration.");
        return nullptr;
    }

    auto let_decl = std::make_unique<LetDeclaration>(std::move(names), std::move(initializers));
    let_decl->is_float_declaration = is_float;
    return let_decl;
}

/**
 * @brief Parses an IF ... THEN/DO ... statement.
 */
StmtPtr Parser::parse_if_statement() {
    TraceGuard guard(*this, "parse_if_statement");
    consume(TokenType::If, "Expect 'IF'.");
    auto condition = parse_expression();
    if (!match(TokenType::Do) && !match(TokenType::Then)) {
        error("Expect 'THEN' or 'DO' after IF condition.");
        return nullptr;
    }
    auto then_branch = parse_statement();
    return std::make_unique<IfStatement>(std::move(condition), std::move(then_branch));
}

/**
 * @brief Parses an UNLESS ... THEN/DO ... statement.
 */
StmtPtr Parser::parse_unless_statement() {
    TraceGuard guard(*this, "parse_unless_statement");
    consume(TokenType::Unless, "Expect 'UNLESS'.");
    auto condition = parse_expression();
    if (!match(TokenType::Do) && !match(TokenType::Then)) {
        error("Expect 'THEN' or 'DO' after UNLESS condition.");
        return nullptr;
    }
    auto then_branch = parse_statement();
    return std::make_unique<UnlessStatement>(std::move(condition), std::move(then_branch));
}

/**
 * @brief Parses a TEST ... THEN ... ELSE ... statement.
 */
StmtPtr Parser::parse_test_statement() {
    TraceGuard guard(*this, "parse_test_statement");
    consume(TokenType::Test, "Expect 'TEST'.");
    auto condition = parse_expression();
    if (!match(TokenType::Do) && !match(TokenType::Then)) {
        error("Expect 'THEN' or 'DO' after TEST condition.");
        return nullptr;
    }
    auto then_branch = parse_statement();
    StmtPtr else_branch = nullptr;
    if (match(TokenType::Else)) {
        else_branch = parse_statement();
    }
    return std::make_unique<TestStatement>(std::move(condition), std::move(then_branch), std::move(else_branch));
}

/**
 * @brief Parses a WHILE ... DO ... statement.
 */
StmtPtr Parser::parse_while_statement() {
    TraceGuard guard(*this, "parse_while_statement");
    consume(TokenType::While, "Expect 'WHILE'.");
    auto condition = parse_expression();
    consume(TokenType::Do, "Expect 'DO' after WHILE condition.");
    auto body = parse_statement();
    return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
}

/**
 * @brief Parses a FOR ... DO ... statement.
 */
StmtPtr Parser::parse_for_statement() {
    TraceGuard guard(*this, "parse_for_statement");
    consume(TokenType::For, "Expect 'FOR'.");
    std::string var_name = current_token_.value;
    consume(TokenType::Identifier, "Expect loop variable name.");
    consume(TokenType::Equal, "Expect '=' after loop variable.");
    auto start_expr = parse_expression();
    consume(TokenType::To, "Expect 'TO' in FOR loop.");
    auto end_expr = parse_expression();
    ExprPtr step_expr = nullptr;
    if (match(TokenType::By)) {
        step_expr = parse_expression();
    } else {
        // If no BY clause, step_expr remains nullptr
        // Default step value will be handled in CFGBuilderPass
    }
    consume(TokenType::Do, "Expect 'DO' in FOR loop.");
    auto body = parse_statement();
    return std::make_unique<ForStatement>(var_name, std::move(start_expr), std::move(end_expr), std::move(body), std::move(step_expr));
}

/**
 * @brief Parses a SWITCHON ... INTO ... statement.
 */
StmtPtr Parser::parse_switchon_statement() {
    TraceGuard guard(*this, "parse_switchon_statement");
    consume(TokenType::Switchon, "Expect 'SWITCHON'.");
    auto expression = parse_expression();
    consume(TokenType::Into, "Expect 'INTO' after SWITCHON expression.");
    consume(TokenType::LBrace, "Expect '$(' or '{' to begin SWITCHON body.");

    std::vector<std::unique_ptr<CaseStatement>> cases;
    std::unique_ptr<DefaultStatement> default_case = nullptr;

    while (!check(TokenType::RBrace) && !is_at_end()) {
        if (match(TokenType::Case)) {
            auto const_expr = parse_expression();
            consume(TokenType::Colon, "Expect ':' after CASE constant.");
            auto command = parse_statement();
            if (!command) break;
            cases.push_back(std::make_unique<CaseStatement>(std::move(const_expr), std::move(command)));
        } else if (match(TokenType::Default)) {
            consume(TokenType::Colon, "Expect ':' after DEFAULT.");
            if (default_case) {
                error("Multiple DEFAULT cases in SWITCHON statement.");
            }
            auto def_stmt = parse_statement();
            if (!def_stmt) break;
            default_case = std::make_unique<DefaultStatement>(std::move(def_stmt));
        } else {
            error("Only CASE or DEFAULT allowed inside SWITCHON body.");
            advance();
        }
    }

    consume(TokenType::RBrace, "Expect '$)' or '}' to end SWITCHON body.");
    return std::make_unique<SwitchonStatement>(std::move(expression), std::move(cases), std::move(default_case));
}

/**
 * @brief Parses a FREEVEC or FREESTR statement.
 */
StmtPtr Parser::parse_free_statement() {
    TraceGuard guard(*this, "parse_free_statement");
    if (match(TokenType::FREEVEC)) {


    }

    auto expr = parse_expression();
    if (!expr) {
        error("Expected an expression after 'FREEVEC' or 'FREESTR'.");
        return nullptr;
    }
    return std::make_unique<FreeStatement>(std::move(expr));
}


// --- Simple Keyword Statements ---

StmtPtr Parser::parse_goto_statement() {
    TraceGuard guard(*this, "parse_goto_statement");
    consume(TokenType::Goto, "Expect 'GOTO'.");
    return std::make_unique<GotoStatement>(parse_expression());
}

StmtPtr Parser::parse_return_statement() {
    TraceGuard guard(*this, "parse_return_statement");
    consume(TokenType::Return, "Expect 'RETURN'.");
    return std::make_unique<ReturnStatement>();
}

StmtPtr Parser::parse_finish_statement() {
    TraceGuard guard(*this, "parse_finish_statement");
    consume(TokenType::Finish, "Expect 'FINISH'.");
    return std::make_unique<FinishStatement>();
}

StmtPtr Parser::parse_break_statement() {
    TraceGuard guard(*this, "parse_break_statement");
    consume(TokenType::Break, "Expect 'BREAK'.");
    return std::make_unique<BreakStatement>();
}

StmtPtr Parser::parse_brk_statement() {
    TraceGuard guard(*this, "parse_brk_statement");
    consume(TokenType::Brk, "Expect 'BRK'.");
    return std::make_unique<BrkStatement>();
}

StmtPtr Parser::parse_loop_statement() {
    TraceGuard guard(*this, "parse_loop_statement");
    consume(TokenType::Loop, "Expect 'LOOP'.");
    return std::make_unique<LoopStatement>();
}

StmtPtr Parser::parse_endcase_statement() {
    TraceGuard guard(*this, "parse_endcase_statement");
    consume(TokenType::Endcase, "Expect 'ENDCASE'.");
    return std::make_unique<EndcaseStatement>();
}

StmtPtr Parser::parse_resultis_statement() {
    TraceGuard guard(*this, "parse_resultis_statement");
    consume(TokenType::Resultis, "Expect 'RESULTIS'.");
    return std::make_unique<ResultisStatement>(parse_expression());
}
