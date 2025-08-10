#ifndef PARSER_H
#define PARSER_H

#include "AST.h"
#include "Lexer.h"
#include <vector>
#include <string>
#include <memory>

class Parser {
public:
    /**
     * @brief Constructs the parser.
     * @param lexer A reference to the lexer that will provide the token stream.
     * @param trace A flag to enable verbose tracing for debugging purposes.
     */
    Parser(Lexer& lexer, bool trace = false);

    /**
     * @brief The main entry point to parse the entire source and build the AST.
     * @return A unique_ptr to the root Program node of the AST.
     */
    ProgramPtr parse_program();

private:
    Lexer& lexer_;
    ProgramPtr program_; // The AST is built directly into this member.

    Token current_token_;
    Token previous_token_;

    bool trace_enabled_;
    int trace_depth_ = 0;
    bool fatal_error_ = false;

    // --- Core Parser Methods ---
    void advance();
    bool match(TokenType type);
    void consume(TokenType expected, const std::string& error_message);
    bool check(TokenType type) const;
    bool is_at_end() const;
    void error(const std::string& message);
    void synchronize();
    bool is_expression_start() const;

    // --- Statement Parsers ---
    // Statements
    StmtPtr parse_statement();
    StmtPtr parse_primary_statement(); // Helper for postfix REPEAT
    StmtPtr parse_if_statement();
    StmtPtr parse_unless_statement();
    StmtPtr parse_test_statement();
    StmtPtr parse_while_statement();
    StmtPtr parse_until_statement();
    StmtPtr parse_for_statement();
    StmtPtr parse_switchon_statement();
    StmtPtr parse_goto_statement();
    StmtPtr parse_free_statement();
    StmtPtr parse_return_statement();
    StmtPtr parse_finish_statement();
    StmtPtr parse_break_statement();
    StmtPtr parse_brk_statement();
    StmtPtr parse_loop_statement();
    StmtPtr parse_endcase_statement();
    StmtPtr parse_resultis_statement();
    StmtPtr parse_let_statement();
    DeclPtr parse_toplevel_let_declaration();
    StmtPtr parse_assignment_or_routine_call();
    StmtPtr parse_block_or_compound_statement();

    // --- Expression Parsers ---
    ExprPtr parse_expression(int precedence = 0);
    ExprPtr parse_unary_expression();
    ExprPtr parse_primary_expression();
    ExprPtr parse_grouped_expression();
    ExprPtr parse_valof_expression();
    ExprPtr parse_fvalof_expression();
    int get_token_precedence(TokenType type);

    // --- Declaration Parsers ---
    bool is_declaration_start();
    DeclPtr parse_declaration();
    DeclPtr parse_label_declaration();
    DeclPtr parse_manifest_declaration();
    DeclPtr parse_static_declaration();
    DeclPtr parse_global_declaration();

    /**
     * @brief Parses a function or routine body after the name and parameters.
     * @param name The name of the function/routine.
     * @param params The list of parameter names.
     * @return A DeclPtr to the created FunctionDeclaration or RoutineDeclaration.
     */
    DeclPtr parse_function_or_routine_body(const std::string& name, std::vector<std::string> params);

    // --- Unified LET Parser ---
    /**
     * @brief The central dispatcher that handles all LET constructs.
     * It uses lookahead to determine if the construct is a function declaration
     * or a variable statement and adds it to the correct list in the Program AST.
     */
    void parse_let_construct();

    // --- Debugging ---
    void trace(const std::string& rule_name);

    // Helper class for RAII-based trace logging
    class TraceGuard {
    public:
        TraceGuard(Parser& parser, const std::string& name);
        ~TraceGuard();
    private:
        bool is_expression_start() const;
        Parser& parser_;
    };
};

#endif // PARSER_H
