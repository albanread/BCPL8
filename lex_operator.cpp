#include "Lexer.h"
#include <string>

Token Lexer::scan_operator() {
    char c = advance();
    switch (c) {
        case '(': return make_token(TokenType::LParen);
        case ')': return make_token(TokenType::RParen);
        case ',': return make_token(TokenType::Comma);
        case ';': return make_token(TokenType::Semicolon);
        case '@': return make_token(TokenType::AddressOf);
        case '!': return make_token(TokenType::VecIndirection);
        case '%': return make_token(TokenType::CharIndirection);
        case '&': return make_token(TokenType::LogicalAnd);
        case '|': return make_token(TokenType::LogicalOr);
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
            break;
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
