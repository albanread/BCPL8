#include "Parser.h"
#include "LexerDebug.h" // For trace logging
#include <stdexcept>
#include <utility> // For std::move

/**
 * Maps a token type to its corresponding BinaryOp::Operator enum.
 */

 static BinaryOp::Operator to_binary_op(TokenType type) {
     switch (type) {
         // Integer Arithmetic
         case TokenType::Plus:           return BinaryOp::Operator::Add;
         case TokenType::Minus:          return BinaryOp::Operator::Subtract;
         case TokenType::Multiply:       return BinaryOp::Operator::Multiply;
         case TokenType::Divide:         return BinaryOp::Operator::Divide;
         case TokenType::Remainder:      return BinaryOp::Operator::Remainder;

         // Equality and Relational
         case TokenType::Equal:          return BinaryOp::Operator::Equal;
         case TokenType::NotEqual:       return BinaryOp::Operator::NotEqual;
         case TokenType::Less:           return BinaryOp::Operator::Less;
         case TokenType::LessEqual:      return BinaryOp::Operator::LessEqual;
         case TokenType::Greater:        return BinaryOp::Operator::Greater;
         case TokenType::GreaterEqual:   return BinaryOp::Operator::GreaterEqual;

         // Logical and Equivalence
         case TokenType::LogicalAnd:     return BinaryOp::Operator::LogicalAnd;
         case TokenType::LogicalOr:      return BinaryOp::Operator::LogicalOr;
         case TokenType::Equivalence:    return BinaryOp::Operator::Equivalence;
         case TokenType::NotEquivalence: return BinaryOp::Operator::NotEquivalence;

         // Bitwise Shifts
         case TokenType::LeftShift:      return BinaryOp::Operator::LeftShift;
         case TokenType::RightShift:     return BinaryOp::Operator::RightShift;

         // Floating-Point Arithmetic
         case TokenType::FloatPlus:      return BinaryOp::Operator::FloatAdd;
         case TokenType::FloatMinus:     return BinaryOp::Operator::FloatSubtract;
         case TokenType::FloatMultiply:  return BinaryOp::Operator::FloatMultiply;
         case TokenType::FloatDivide:    return BinaryOp::Operator::FloatDivide;

         // Floating-Point Comparison
         case TokenType::FloatEqual:     return BinaryOp::Operator::FloatEqual;
         case TokenType::FloatNotEqual:  return BinaryOp::Operator::FloatNotEqual;
         case TokenType::FloatLess:      return BinaryOp::Operator::FloatLess;
         case TokenType::FloatLessEqual: return BinaryOp::Operator::FloatLessEqual;
         case TokenType::FloatGreater:   return BinaryOp::Operator::FloatGreater;
         case TokenType::FloatGreaterEqual: return BinaryOp::Operator::FloatGreaterEqual;

         default:
             throw std::logic_error("Invalid token type for a binary operator.");
     }
 }


/**
 * @brief Constructs the parser, primes the first token, and initializes state.
 * @param lexer A reference to the lexer providing the token stream.
 * @param trace Flag to enable debug tracing.
 */
Parser::Parser(Lexer& lexer, bool trace)
    : lexer_(lexer),
      trace_enabled_(trace),
      fatal_error_(false),
      trace_depth_(0) {
    // Prime the pump by advancing to the first token.
    advance();
}


/**
 * @brief Main entry point. Parses the entire token stream and returns the complete AST.
 */
ProgramPtr Parser::parse_program() {
    TraceGuard guard(*this, "parse_program");
    program_ = std::make_unique<Program>();

    while (!is_at_end() && !fatal_error_) {
        try {
            if (check(TokenType::Let) || check(TokenType::FLet)) {
                // At the top level, LET/FLET can be a function or a global variable.
                // Both are considered Declarations.
                program_->declarations.push_back(parse_toplevel_let_declaration());
            } else if (is_declaration_start()) {
                program_->declarations.push_back(parse_declaration());
            } else {
                error("Executable statements are not allowed in the global scope.");
                synchronize();
                break; // Exit on finding a statement at the top level.
            }
        } catch (const std::runtime_error&) {
            // In case of an error, synchronize to the next likely statement/declaration.
            synchronize();
        }
    }
    return std::move(program_);
}

// --- New: parse_toplevel_let_declaration for top-level LET/FLET handling --- //
DeclPtr Parser::parse_toplevel_let_declaration() {
    TraceGuard guard(*this, "parse_toplevel_let_declaration");

    bool is_float = check(TokenType::FLet);
    advance(); // Consume LET or FLET

    std::string name = current_token_.value;
    consume(TokenType::Identifier, "Expect identifier after LET/FLET.");

    if (check(TokenType::LParen)) {
        // This is a function declaration, which is already a DeclPtr.
        std::vector<std::string> params;
        consume(TokenType::LParen, "Expect '(' after function name.");
        if (!check(TokenType::RParen)) {
            do {
                params.push_back(current_token_.value);
                consume(TokenType::Identifier, "Expect parameter name.");
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RParen, "Expect ')' after function parameters.");
        return parse_function_or_routine_body(name, std::move(params));
    } else {
        // This is a global variable declaration. Use the new AST node.
        std::vector<std::string> names;
        names.push_back(name);
        while (match(TokenType::Comma)) {
            names.push_back(current_token_.value);
            consume(TokenType::Identifier, "Expect identifier after comma in LET/FLET.");
        }

        consume(TokenType::Equal, "Expect '=' after name(s).");

        std::vector<ExprPtr> initializers;
        do {
            initializers.push_back(parse_expression());
        } while (match(TokenType::Comma));

        if (names.size() != initializers.size()) {
            error("Mismatch between number of names and initializers in global LET/FLET declaration.");
        }

        auto global_decl = std::make_unique<GlobalVariableDeclaration>(std::move(names), std::move(initializers));
        global_decl->is_float_declaration = is_float;
        return global_decl;
    }
}

/**
 * @brief The central dispatcher for all LET/FLET constructs.
 * This is the core of the new parser logic. It uses lookahead to decide
 * whether to parse a function declaration or a variable statement.
 */
void Parser::parse_let_construct() {
    TraceGuard guard(*this, "parse_let_construct");

    bool is_float_declaration = check(TokenType::FLet);
    advance(); // Consume LET or FLET

    std::string name = current_token_.value;
    consume(TokenType::Identifier, "Expect identifier after LET/FLET.");

    // --- Lookahead Logic ---
    if (check(TokenType::LParen)) {
        // Sequence is 'LET IDENTIFIER (', so this is a function/routine declaration.
        std::vector<std::string> params;
        consume(TokenType::LParen, "Expect '(' after function name.");
        if (!check(TokenType::RParen)) {
            do {
                params.push_back(current_token_.value);
                consume(TokenType::Identifier, "Expect parameter name.");
            } while (match(TokenType::Comma));
        }
        consume(TokenType::RParen, "Expect ')' after function parameters.");

        // Parse the body and add the resulting node to the declarations list.
        program_->declarations.push_back(parse_function_or_routine_body(name, std::move(params)));
    } else {
        // Sequence is 'LET IDENTIFIER =', so it's a variable assignment statement.
        std::vector<std::string> names;
        names.push_back(name);
        while (match(TokenType::Comma)) {
            names.push_back(current_token_.value);
            consume(TokenType::Identifier, "Expect identifier after comma in LET/FLET list.");
        }

        consume(TokenType::Equal, "Expect '=' after name(s) in LET/FLET declaration.");

        std::vector<ExprPtr> initializers;
        do {
            initializers.push_back(parse_expression());
        } while (match(TokenType::Comma));

        if (names.size() != initializers.size()) {
            error("Mismatch between number of names and initializers in LET/FLET declaration.");
            return;
        }

        auto let_decl = std::make_unique<LetDeclaration>(std::move(names), std::move(initializers));
        let_decl->is_float_declaration = is_float_declaration;

        // Add the resulting node to the statements list.
        program_->statements.push_back(std::move(let_decl));
    }
}

/**
 * @brief Parses the body of a function or routine after the name and parameters have been consumed.
 */
DeclPtr Parser::parse_function_or_routine_body(const std::string& name, std::vector<std::string> params) {
    TraceGuard guard(*this, "parse_function_or_routine_body");

    if (match(TokenType::Equal)) {
        // Function with a return value (body is an expression).
        auto body = parse_expression();
        return std::make_unique<FunctionDeclaration>(name, std::move(params), std::move(body));
    }
    if (match(TokenType::Be)) {
        // DEVELOPER NOTE:
        // The keyword `ROUTINE` does not exist in the BCPL source language.
        // This rule parses the `LET name() BE <statement>` syntax and creates an
        // internal `RoutineDeclaration` AST node. This distinction between the
        // source syntax and the compiler's internal representation is important
        // to avoid confusion.
        // Routine without a return value (body is a statement).
        auto body = parse_statement();
        return std::make_unique<RoutineDeclaration>(name, std::move(params), std::move(body));
    }

    error("Expect '=' or 'BE' in function/routine declaration.");
    return nullptr;
}


// --- Core Parser Methods (Implementations) ---

void Parser::advance() {
    previous_token_ = std::move(current_token_);
    while (true) {
        current_token_ = lexer_.get_next_token();
        if (current_token_.type != TokenType::Error) break;
        error("Lexical error: " + current_token_.value);
    }
}

bool Parser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

void Parser::consume(TokenType expected, const std::string& error_message) {
    if (check(expected)) {
        advance();
        return;
    }
    // New, more informative error generation
    std::string detailed_message = "Expected token " + to_string(expected) +
                                   " but got " + to_string(current_token_.type) +
                                   ". " + error_message;
    error(detailed_message); // Call our enhanced error method
    throw std::runtime_error("Parsing error.");
}

bool Parser::check(TokenType type) const {
    if (is_at_end()) return false;
    return current_token_.type == type;
}

bool Parser::is_at_end() const {
    return current_token_.type == TokenType::Eof;
}

void Parser::error(const std::string& message) {
    std::string error_msg = "[L" + std::to_string(previous_token_.line) +
                            " C" + std::to_string(previous_token_.column) +
                            "] Error: " + message;
    errors_.push_back(error_msg); // Accumulate error messages
    LexerTrace(error_msg);
    fatal_error_ = true;
}

void Parser::synchronize() {
    advance();
    while (!is_at_end()) {
        if (previous_token_.type == TokenType::Semicolon) return;
        switch (current_token_.type) {
            case TokenType::Let:
            case TokenType::Function:
            case TokenType::Routine:
            case TokenType::If:
            case TokenType::For:
            case TokenType::While:
            case TokenType::Return:
                return; // Return on tokens that can start a new statement/declaration.
            default:
                break;
        }
        advance();
    }
}

void Parser::trace(const std::string& rule_name) {
    if (!trace_enabled_) return;
    std::string indent(trace_depth_ * 2, ' ');
    LexerTrace(indent + rule_name);
}

Parser::TraceGuard::TraceGuard(Parser& parser, const std::string& name) : parser_(parser) {
    parser_.trace(">> Entering " + name);
    parser_.trace_depth_++;
}

Parser::TraceGuard::~TraceGuard() {
    parser_.trace_depth_--;
    parser_.trace("<< Exiting");
}
