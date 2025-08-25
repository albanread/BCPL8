#include "Lexer.h"
#include <string>
#include <cctype>

Token Lexer::scan_operator() {
    char c = advance();

    // Handle textual "OR" as LogicalOr
    if (c == 'O' || c == 'o') {
        if ((peek_char() == 'R' || peek_char() == 'r')) {
            advance();
            // Make sure it's not part of a longer identifier
            if (!std::isalnum(peek_char()) && peek_char() != '_') {
                return make_token(TokenType::LogicalOr);
            }
        }
    }

    switch (c) {
        case '(': return make_token(TokenType::LParen);
        case ')': return make_token(TokenType::RParen);
        case ',': return make_token(TokenType::Comma);
        case ';': return make_token(TokenType::Semicolon);
        case '@': return make_token(TokenType::AddressOf);
        case '!':
            if (last_token_was_value_) {
                // Infix: variable!expression
                return make_token(TokenType::VecIndirection);
            } else {
                // Prefix: !variable
                return make_token(TokenType::Indirection);
            }
        case '%':
            if (peek_char() == '%') {
                advance(); // Consume the second '%'
                return make_token(TokenType::Bitfield);
            }
            if (last_token_was_value_) {
                // Infix: variable%expression
                return make_token(TokenType::CharVectorIndirection);
            } else {
                // Prefix: %variable
                return make_token(TokenType::CharIndirection);
            }
        case '&':
            if (peek_char() == '&') {
                advance();
                return make_token(TokenType::LogicalAnd);
            }
            return make_token(TokenType::BitwiseAnd);
        case '|':
            if (peek_char() == '|') {
                advance();
                return make_token(TokenType::LogicalOr);
            }
            return make_token(TokenType::BitwiseOr);
        case '+':
            if (peek_char() == '#') { advance(); return make_token(TokenType::FloatPlus); }
            return make_token(TokenType::Plus);
        case '*':
            if (peek_char() == '#') { advance(); return make_token(TokenType::FloatMultiply); }
            return make_token(TokenType::Multiply);
        case '/':
            if (peek_char() == '#') { advance(); return make_token(TokenType::FloatDivide); }
            return make_token(TokenType::Divide);
        case ':':
            if (peek_char() == '=') { advance(); return make_token(TokenType::Assign); }
            return make_token(TokenType::Colon);
        case '~':
            if (peek_char() == '=') {
                advance();
                if (peek_char() == '#') { advance(); return make_token(TokenType::FloatNotEqual); }
                return make_token(TokenType::NotEqual);
            }
            // Standalone '~' is BitwiseNot
            return make_token(TokenType::BitwiseNot);
        case '<':
            if (peek_char() == '=') {
                advance();
                if (peek_char() == '#') { advance(); return make_token(TokenType::FloatLessEqual); }
                return make_token(TokenType::LessEqual);
            }
            if (peek_char() == '<') { advance(); return make_token(TokenType::LeftShift); }
            if (peek_char() == '#') { advance(); return make_token(TokenType::FloatLess); }
            return make_token(TokenType::Less);
        case '>':
            if (peek_char() == '=') {
                advance();
                if (peek_char() == '#') { advance(); return make_token(TokenType::FloatGreaterEqual); }
                return make_token(TokenType::GreaterEqual);
            }
            if (peek_char() == '>') { advance(); return make_token(TokenType::RightShift); }
            if (peek_char() == '#') { advance(); return make_token(TokenType::FloatGreater); }
            return make_token(TokenType::Greater);
        case '=':
             if (peek_char() == '#') { advance(); return make_token(TokenType::FloatEqual); }
             return make_token(TokenType::Equal);
        case '-':
            if (peek_char() == '>') { advance(); return make_token(TokenType::Conditional); }
            if (peek_char() == '#') { advance(); return make_token(TokenType::FloatMinus); }
            return make_token(TokenType::Minus);
        case '$':
            if (peek_char() == '(') { advance(); return make_token(TokenType::LBrace); }
            break;
        case '#':
             if (peek_char() == '%') { advance(); return make_token(TokenType::FloatVecIndirection); }
             break;
    }
    return make_error_token("Unexpected character: " + std::string(1, c));
}
