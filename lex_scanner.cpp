#include "Lexer.h"
#include "LexerDebug.h"
#include <cctype>

Token Lexer::peek() const {
    Lexer temp_lexer = *this;
    return temp_lexer.get_next_token();
}

char Lexer::advance() {
    if (!is_at_end()) {
        char current_char = source_[position_++];
        if (current_char == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        return current_char;
    }
    return '\0';
}

char Lexer::peek_char() const {
    if (is_at_end()) {
        return '\0';
    }
    return source_[position_];
}

char Lexer::peek_next_char() const {
    if (position_ + 1 >= source_.length()) {
        return '\0';
    }
    return source_[position_ + 1];
}

bool Lexer::is_at_end() const {
    return position_ >= source_.length();
}

void Lexer::skip_whitespace_and_comments() {
    while (!is_at_end()) {
        char c = peek_char();
        if (std::isspace(c)) {
            advance();
            continue;
        }
        if (c == '/' && peek_next_char() == '/') {
            while (peek_char() != '\n' && !is_at_end()) {
                advance();
            }
            continue;
        }
        if (c == '/' && peek_next_char() == '*') {
            advance();
            advance();
            while (!is_at_end() && !(peek_char() == '*' && peek_next_char() == '/')) {
                advance();
            }
            if (!is_at_end()) {
                advance();
                advance();
            }
            continue;
        }
        break;
    }
}

Token Lexer::make_token(TokenType type) const {
    Token token = {type, "", line_, column_};
    if (trace_enabled_) {
        LexerTrace("Made token: " + token.to_string());
    }
    return token;
}

Token Lexer::make_token_with_value(TokenType type, const std::string& value) const {
    Token token = {type, value, line_, column_};
     if (trace_enabled_) {
        LexerTrace("Made token with value: " + token.to_string());
    }
    return token;
}

Token Lexer::make_error_token(const std::string& message) const {
    Token token = {TokenType::Error, message, line_, column_};
     if (trace_enabled_) {
        LexerTrace("Made ERROR token: " + token.to_string());
    }
    return token;
}