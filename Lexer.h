#ifndef LEXER_H
#define LEXER_H

#include <string>
#include <vector>
#include <cstdint>
#include <unordered_map>

enum class TokenType {
    Eof, Error,
    Let, Manifest, Static, Global, Function, Routine, And,
    Vec, If, Unless, Test, While, Until, Repeat, For, Switchon,
    Case, Default, Goto, Return, Finish, Loop, Endcase, Resultis,
    Valof, FValof, Be, To, By, Into, Do, Or, Break, Get, FLet,
    Then, Else,
    String, Brk, FREEVEC,
    Identifier, NumberLiteral, StringLiteral, CharLiteral, BooleanLiteral,
    Assign, Plus, Minus, Multiply, Divide, Remainder, Equal, NotEqual,
    Less, LessEqual, Greater, GreaterEqual, LogicalAnd, LogicalOr,
    Equivalence, NotEquivalence, LeftShift, RightShift, Conditional,
    AddressOf, Indirection, VecIndirection, CharIndirection,
    FloatPlus, FloatMinus, FloatMultiply, FloatDivide, FloatVecIndirection,
    FloatEqual, FloatNotEqual, FloatLess, FloatLessEqual, FloatGreater, FloatGreaterEqual,
    FLOAT,
    LParen, RParen, LBrace, RBrace, Comma, Semicolon, Colon,
};

std::string to_string(TokenType type);

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    std::string to_string() const;
};

class Lexer {
public:
    Lexer(std::string source, bool trace = false);
    Token get_next_token();
    Token peek() const;

public:
    std::string source_;
private:
    size_t position_;
    int line_;
    int column_;
    bool trace_enabled_;
    static const std::unordered_map<std::string, TokenType> keywords_;

    char advance();
    char peek_char() const;
    char peek_next_char() const;
    bool is_at_end() const;
    void skip_whitespace_and_comments();

    Token make_token(TokenType type) const;
    Token make_error_token(const std::string& message) const;
    Token scan_identifier_or_keyword();
    Token scan_number();
    Token scan_string();
    Token scan_char();
    Token scan_operator();
    Token make_token_with_value(TokenType type, const std::string& value) const;
};

#endif // LEXER_H