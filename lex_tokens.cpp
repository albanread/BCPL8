#include "Lexer.h"
#include <cctype>
#include <string>

Token Lexer::scan_identifier_or_keyword() {
    std::string value;
    int start_col = column_;
    while (std::isalnum(peek_char()) || peek_char() == '_') {
        value += advance();
    }

    auto it = keywords_.find(value);
    if (it != keywords_.end()) {
        return {it->second, value, line_, start_col};
    }
    return {TokenType::Identifier, value, line_, start_col};
}

Token Lexer::scan_number() {
    std::string value;
    int start_col = column_;
    bool is_float = false;

    if (peek_char() == '#') {
        value += advance();
        if (peek_char() == 'X' || peek_char() == 'x') {
            value += advance();
            while (std::isxdigit(peek_char())) {
                value += advance();
            }
            return {TokenType::NumberLiteral, value, line_, start_col};
        }
        while (peek_char() >= '0' && peek_char() <= '7') {
            value += advance();
        }
        return {TokenType::NumberLiteral, value, line_, start_col};
    }

    while (std::isdigit(peek_char())) {
        value += advance();
    }

    if (peek_char() == '.') {
        is_float = true;
        value += advance();
        while (std::isdigit(peek_char())) {
            value += advance();
        }
    }

    if (peek_char() == 'e' || peek_char() == 'E') {
        is_float = true;
        value += advance();
        if (peek_char() == '+' || peek_char() == '-') {
            value += advance();
        }
        while (std::isdigit(peek_char())) {
            value += advance();
        }
    }

    return {TokenType::NumberLiteral, value, line_, start_col};
}

Token Lexer::scan_string() {
    std::string value;
    int start_col = column_;
    advance();

    while (peek_char() != '"' && !is_at_end()) {
        char c = advance();
        if (c == '*') {
            switch (peek_char()) {
                case 'n': case 'N': value += '\n'; advance(); break;
                case 't': case 'T': value += '\t'; advance(); break;
                case 's': case 'S': value += ' '; advance(); break;
                case 'b': case 'B': value += '\b'; advance(); break;
                case 'p': case 'P': value += '\f'; advance(); break;
                case 'c': case 'C': value += '\r'; advance(); break;
                case '"': value += '"'; advance(); break;
                case '*': value += '*'; advance(); break;
                default: value += '*'; break;
            }
        } else {
            value += c;
        }
    }

    if (is_at_end()) {
        return make_error_token("Unterminated string literal.");
    }

    advance();
    return {TokenType::StringLiteral, value, line_, start_col};
}

Token Lexer::scan_char() {
    std::string value;
    int start_col = column_;
    advance();

    if (peek_char() == '\'') {
        return make_error_token("Empty character literal.");
    }

    char c = advance();
    if (c == '*') {
        switch (peek_char()) {
            case 'n': case 'N': value += '\n'; advance(); break;
            default: value += advance(); break;
        }
    } else {
        value += c;
    }

    if (peek_char() != '\'') {
        return make_error_token("Multi-character literal, or unterminated char literal.");
    }
    advance();

    return {TokenType::CharLiteral, value, line_, start_col};
}