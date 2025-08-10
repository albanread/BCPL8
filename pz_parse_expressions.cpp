#include "Parser.h"
#include <stdexcept>
#include <vector>

/**
 * @brief Maps a token type to its corresponding BinaryOp::Operator enum.
 */
static BinaryOp::Operator to_binary_op(TokenType type) {
    switch (type) {
        case TokenType::Plus:           return BinaryOp::Operator::Add;
        case TokenType::Minus:          return BinaryOp::Operator::Subtract;
        case TokenType::Multiply:       return BinaryOp::Operator::Multiply;
        case TokenType::Divide:         return BinaryOp::Operator::Divide;
        case TokenType::Remainder:      return BinaryOp::Operator::Remainder;
        case TokenType::Equal:          return BinaryOp::Operator::Equal;
        case TokenType::NotEqual:       return BinaryOp::Operator::NotEqual;
        case TokenType::Less:           return BinaryOp::Operator::Less;
        case TokenType::LessEqual:      return BinaryOp::Operator::LessEqual;
        case TokenType::Greater:        return BinaryOp::Operator::Greater;
        case TokenType::GreaterEqual:   return BinaryOp::Operator::GreaterEqual;
        case TokenType::LogicalAnd:     return BinaryOp::Operator::LogicalAnd;
        case TokenType::LogicalOr:      return BinaryOp::Operator::LogicalOr;
        case TokenType::Equivalence:    return BinaryOp::Operator::Equivalence;
        case TokenType::NotEquivalence: return BinaryOp::Operator::NotEquivalence;
        case TokenType::LeftShift:      return BinaryOp::Operator::LeftShift;
        case TokenType::RightShift:     return BinaryOp::Operator::RightShift;
        case TokenType::FloatPlus:      return BinaryOp::Operator::FloatAdd;
        case TokenType::FloatMinus:     return BinaryOp::Operator::FloatSubtract;
        case TokenType::FloatMultiply:  return BinaryOp::Operator::FloatMultiply;
        case TokenType::FloatDivide:    return BinaryOp::Operator::FloatDivide;
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
 * @brief Returns the binding power (precedence) of a token. Higher numbers mean higher precedence.
 */
int Parser::get_token_precedence(TokenType type) {
    switch (type) {
        case TokenType::Conditional:        return 1;
        case TokenType::LogicalOr:          return 2;
        case TokenType::LogicalAnd:         return 3;
        case TokenType::Equivalence:
        case TokenType::NotEquivalence:     return 4;
        case TokenType::Equal: case TokenType::NotEqual:
        case TokenType::Less:  case TokenType::LessEqual:
        case TokenType::Greater: case TokenType::GreaterEqual:
        case TokenType::FloatEqual: case TokenType::FloatNotEqual:
        case TokenType::FloatLess: case TokenType::FloatLessEqual:
        case TokenType::FloatGreater: case TokenType::FloatGreaterEqual:
            return 5;
        case TokenType::LeftShift: case TokenType::RightShift:
            return 6;
        case TokenType::Plus: case TokenType::Minus:
        case TokenType::FloatPlus: case TokenType::FloatMinus:
            return 7;
        case TokenType::Multiply: case TokenType::Divide: case TokenType::Remainder:
        case TokenType::FloatMultiply: case TokenType::FloatDivide:
            return 8;
        // Postfix operators have the highest precedence.
        case TokenType::LParen:
        case TokenType::VecIndirection:
        case TokenType::CharIndirection:
        case TokenType::FloatVecIndirection:
            return 9;
        default:
            return 0; // Not an operator
    }
}

/**
 * @brief Parses an expression using a Pratt parser (precedence climbing) algorithm.
 * @param precedence The current precedence level.
 */
ExprPtr Parser::parse_expression(int precedence) {
    TraceGuard guard(*this, "parse_expression");
    ExprPtr left;

    // --- Prefix Operators ---
    if (match(TokenType::AddressOf)) {
        left = std::make_unique<UnaryOp>(UnaryOp::Operator::AddressOf, parse_expression(8));
    } else if (match(TokenType::Indirection)) {
        left = std::make_unique<UnaryOp>(UnaryOp::Operator::Indirection, parse_expression(8));
    } else if (match(TokenType::Minus)) {
        left = std::make_unique<UnaryOp>(UnaryOp::Operator::Negate, parse_expression(8));
    } else if (match(TokenType::FLOAT)) {
        left = std::make_unique<UnaryOp>(UnaryOp::Operator::FloatConvert, parse_expression(8));
    } else {
        left = parse_primary_expression();
    }

    // --- Infix and Postfix Operators ---
    while (left && precedence < get_token_precedence(current_token_.type)) {
        TokenType type = current_token_.type;
        advance();

        if (type == TokenType::LParen) { // Function Call
            std::vector<ExprPtr> args;
            if (!check(TokenType::RParen)) {
                do { args.push_back(parse_expression()); } while (match(TokenType::Comma));
            }
            consume(TokenType::RParen, "Expect ')' after function arguments.");
            left = std::make_unique<FunctionCall>(std::move(left), std::move(args));
        } else if (type == TokenType::VecIndirection) { // Vector Access
            left = std::make_unique<VectorAccess>(std::move(left), parse_expression(9));
        } else if (type == TokenType::CharIndirection) { // Character Indirection
            left = std::make_unique<CharIndirection>(std::move(left), parse_expression(9));
        } else if (type == TokenType::FloatVecIndirection) { // Float Vector Indirection
            left = std::make_unique<FloatVectorIndirection>(std::move(left), parse_expression(9));
        } else if (type == TokenType::Conditional) { // Ternary Conditional
            ExprPtr true_expr = parse_expression();
            consume(TokenType::Comma, "Expect ',' in conditional expression.");
            ExprPtr false_expr = parse_expression(1);
            left = std::make_unique<ConditionalExpression>(std::move(left), std::move(true_expr), std::move(false_expr));
        } else { // Binary Operator
            auto op = to_binary_op(type);
            int next_precedence = get_token_precedence(type);
            ExprPtr right = parse_expression(next_precedence);
            if (!right) {
                error("Expected an expression for right operand of binary operator.");
                return nullptr;
            }
            left = std::make_unique<BinaryOp>(op, std::move(left), std::move(right));
        }
    }
    return left;
}

/**
 * @brief Parses the "atoms" of an expression (literals, variables, etc.).
 */
ExprPtr Parser::parse_primary_expression() {
    TraceGuard guard(*this, "parse_primary_expression");
    if (match(TokenType::NumberLiteral)) {
        const std::string& val_str = previous_token_.value;
        if (val_str.find('.') != std::string::npos || val_str.find('e') != std::string::npos || val_str.find('E') != std::string::npos) {
            return std::make_unique<NumberLiteral>(std::stod(val_str));
        } else {
            return std::make_unique<NumberLiteral>(std::stoll(val_str));
        }
    }
    if (match(TokenType::StringLiteral)) {
        return std::make_unique<StringLiteral>(previous_token_.value);
    }
    if (match(TokenType::CharLiteral)) {
        return std::make_unique<CharLiteral>(previous_token_.value[0]);
    }
    if (match(TokenType::BooleanLiteral)) {
        return std::make_unique<BooleanLiteral>(previous_token_.value == "TRUE");
    }
    if (check(TokenType::Identifier)) {
        auto var = std::make_unique<VariableAccess>(current_token_.value);
        advance();
        return var;
    }
    if (match(TokenType::LParen)) {
        return parse_grouped_expression();
    }
    if (match(TokenType::Valof)) {
        return parse_valof_expression();
    }
    if (match(TokenType::FValof)) {
        return parse_fvalof_expression();
    }
    if (match(TokenType::Vec)) {
        return std::make_unique<VecAllocationExpression>(parse_expression());
    }
    if (match(TokenType::String)) {
        return std::make_unique<StringAllocationExpression>(parse_expression());
    }

    error("Expected an expression.");
    return nullptr;
}

/**
 * @brief Parses a parenthesized expression.
 */
ExprPtr Parser::parse_grouped_expression() {
    TraceGuard guard(*this, "parse_grouped_expression");
    ExprPtr expr = parse_expression();
    consume(TokenType::RParen, "Expect ')' after expression.");
    return expr;
}

/**
 * @brief Parses a VALOF block.
 */
ExprPtr Parser::parse_valof_expression() {
    TraceGuard guard(*this, "parse_valof_expression");
    StmtPtr body = parse_statement();
    return std::make_unique<ValofExpression>(std::move(body));
}

/**
 * @brief Parses an FVALOF block.
 */
ExprPtr Parser::parse_fvalof_expression() {
    TraceGuard guard(*this, "parse_fvalof_expression");
    StmtPtr body = parse_statement();
    return std::make_unique<FloatValofExpression>(std::move(body));
}
