#include "Lexer.h"
#include "LexerDebug.h"
#include <cctype>
#include <utility>

const std::unordered_map<std::string, TokenType> Lexer::keywords_ = {
    {"LET", TokenType::Let}, {"MANIFEST", TokenType::Manifest}, {"STATIC", TokenType::Static},
    {"GLOBAL", TokenType::Global}, {"FUNCTION", TokenType::Function}, {"ROUTINE", TokenType::Routine},
    {"AND", TokenType::LogicalAnd}, {"VEC", TokenType::Vec}, {"IF", TokenType::If},
    {"UNLESS", TokenType::Unless}, {"TEST", TokenType::Test}, {"WHILE", TokenType::While},
    {"UNTIL", TokenType::Until}, {"REPEAT", TokenType::Repeat}, {"FOR", TokenType::For},
    {"SWITCHON", TokenType::Switchon}, {"CASE", TokenType::Case}, {"DEFAULT", TokenType::Default},
    {"GOTO", TokenType::Goto}, {"RETURN", TokenType::Return}, {"FINISH", TokenType::Finish},
    {"FLOAT", TokenType::FLOAT}, {"FLET", TokenType::FLet},
    {"LOOP", TokenType::Loop}, {"ENDCASE", TokenType::Endcase}, {"RESULTIS", TokenType::Resultis},
    {"VALOF", TokenType::Valof}, {"FVALOF", TokenType::FValof}, {"BE", TokenType::Be}, {"TO", TokenType::To},
    {"BY", TokenType::By}, {"INTO", TokenType::Into}, {"DO", TokenType::Do}, {"THEN", TokenType::Then}, {"ELSE", TokenType::Else},
    {"FREEVEC", TokenType::FREEVEC},
    {"OR", TokenType::Or}, {"TRUE", TokenType::BooleanLiteral}, {"FALSE", TokenType::BooleanLiteral},
    {"REM", TokenType::Remainder}, {"EQV", TokenType::Equivalence}, {"NEQV", TokenType::NotEquivalence},
    {"BREAK", TokenType::Break}, {"GET", TokenType::Get},
    {"STRING", TokenType::String}, {"BRK", TokenType::Brk}
};

Lexer::Lexer(std::string source, bool trace)
    : source_(std::move(source)),
      position_(0),
      line_(1),
      column_(1),
      trace_enabled_(trace) {
    if (trace_enabled_) {
        LexerTrace("Lexer initialized. Trace enabled.");
    }
}

Token Lexer::get_next_token() {
    skip_whitespace_and_comments();

    if (is_at_end()) {
        return make_token(TokenType::Eof);
    }

    char current_char = peek_char();

    // CORRECTED: Handle multi-character and single-character block delimiters first.
    if (current_char == '$' && peek_next_char() == '(') {
        advance(); advance();
        return make_token(TokenType::LBrace);
    }
    if (current_char == '$' && peek_next_char() == ')') {
        advance(); advance();
        return make_token(TokenType::RBrace);
    }
    if (current_char == '{') {
        advance();
        return make_token(TokenType::LBrace);
    }
    if (current_char == '}') {
        advance();
        return make_token(TokenType::RBrace);
    }

    // Now, handle other token types.
    if (std::isalpha(current_char) || current_char == '_') {
        return scan_identifier_or_keyword();
    }
    if (std::isdigit(current_char) || current_char == '#') {
        return scan_number();
    }
    if (current_char == '"') {
        return scan_string();
    }
    if (current_char == '\'') {
        return scan_char();
    }

    return scan_operator();
}
