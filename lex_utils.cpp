#include "Lexer.h"
#include <string>

std::string to_string(TokenType type) {
    switch (type) {
        case TokenType::Eof: return "Eof"; case TokenType::Error: return "Error";
        case TokenType::Let: return "Let"; case TokenType::Manifest: return "Manifest";
        case TokenType::Static: return "Static"; case TokenType::Global: return "Global";
        case TokenType::Function: return "Function"; case TokenType::Routine: return "Routine";
        case TokenType::And: return "And"; case TokenType::Vec: return "Vec";
        case TokenType::If: return "If"; case TokenType::Unless: return "Unless";
        case TokenType::Test: return "Test"; case TokenType::While: return "While";
        case TokenType::Until: return "Until"; case TokenType::Repeat: return "Repeat";
        case TokenType::For: return "For"; case TokenType::Switchon: return "Switchon";
        case TokenType::FREEVEC: return "FreeVec";
        case TokenType::Case: return "Case"; case TokenType::Default: return "Default";
        case TokenType::Goto: return "Goto"; case TokenType::Return: return "Return";
        case TokenType::Finish: return "Finish"; case TokenType::Loop: return "Loop";
        case TokenType::Endcase: return "Endcase"; case TokenType::Resultis: return "Resultis";
        case TokenType::Valof: return "Valof"; case TokenType::Be: return "Be";
        case TokenType::To: return "To"; case TokenType::By: return "By";
        case TokenType::Into: return "Into"; case TokenType::Do: return "Do";
        case TokenType::Or: return "Or"; case TokenType::Break: return "Break";
        case TokenType::String: return "String"; case TokenType::Brk: return "Brk";
        case TokenType::Identifier: return "Identifier"; case TokenType::NumberLiteral: return "NumberLiteral";
        case TokenType::StringLiteral: return "StringLiteral"; case TokenType::CharLiteral: return "CharLiteral";
        case TokenType::BooleanLiteral: return "BooleanLiteral"; case TokenType::Assign: return "Assign";
        case TokenType::Plus: return "Plus"; case TokenType::Minus: return "Minus";
        case TokenType::Multiply: return "Multiply"; case TokenType::Divide: return "Divide";
        case TokenType::Remainder: return "Remainder"; case TokenType::Equal: return "Equal";
        case TokenType::NotEqual: return "NotEqual"; case TokenType::Less: return "Less";
        case TokenType::LessEqual: return "LessEqual"; case TokenType::Greater: return "Greater";
        case TokenType::GreaterEqual: return "GreaterEqual"; case TokenType::LogicalAnd: return "LogicalAnd";
        case TokenType::LogicalOr: return "LogicalOr"; case TokenType::Equivalence: return "Equivalence";
        case TokenType::NotEquivalence: return "NotEquivalence"; case TokenType::LeftShift: return "LeftShift";
        case TokenType::RightShift: return "RightShift"; case TokenType::Conditional: return "Conditional";
        case TokenType::AddressOf: return "AddressOf"; case TokenType::Indirection: return "Indirection";
        case TokenType::VecIndirection: return "VecIndirection"; case TokenType::CharIndirection: return "CharIndirection";
        case TokenType::FloatPlus: return "FloatPlus"; case TokenType::FloatMinus: return "FloatMinus";
        case TokenType::FloatMultiply: return "FloatMultiply"; case TokenType::FloatDivide: return "FloatDivide";
        case TokenType::FloatVecIndirection: return "FloatVecIndirection"; case TokenType::FloatEqual: return "FloatEqual";
        case TokenType::FloatNotEqual: return "FloatNotEqual"; case TokenType::FloatLess: return "FloatLess";
        case TokenType::FloatLessEqual: return "FloatLessEqual"; case TokenType::FloatGreater: return "FloatGreater";
        case TokenType::FloatGreaterEqual: return "FloatGreaterEqual"; case TokenType::LParen: return "LParen";
        case TokenType::RParen: return "RParen"; case TokenType::LBrace: return "LBrace";
        case TokenType::RBrace: return "RBrace"; case TokenType::Comma: return "Comma";
        case TokenType::Semicolon: return "Semicolon"; case TokenType::Colon: return "Colon";
        default: return "Unknown";
    }
}

std::string Token::to_string() const {
    return "Token(" + ::to_string(type) + ", '" + value + "', L" + std::to_string(line) + " C" + std::to_string(column) + ")";
}