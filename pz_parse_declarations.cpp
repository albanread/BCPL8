#include "Parser.h"
#include <stdexcept>
#include <vector>

/**
 * @brief Checks if the current token can start a non-LET declaration.
 * This is used by the main parsing loop to decide whether to call parse_declaration.
 */
bool Parser::is_declaration_start() {
    // --- START OF FIX ---
    // A label is now considered a statement, so we remove the check from here.
    // if (check(TokenType::Identifier) && lexer_.peek().type == TokenType::Colon) {
    //     return true;
    // }
    // --- END OF FIX ---
    // Check for other declaration-starting keywords.
    switch (current_token_.type) {
        case TokenType::Manifest:
        case TokenType::Static:
        case TokenType::Global:
        case TokenType::Routine:
        case TokenType::Function:
            return true;
        default:
            return false;
    }
}

/**
 * @brief Parses any non-LET declaration based on the current token.
 */
DeclPtr Parser::parse_declaration() {
    TraceGuard guard(*this, "parse_declaration");

    // Check for a label declaration first, as it starts with an identifier.
    if (check(TokenType::Identifier) && lexer_.peek().type == TokenType::Colon) {
        return parse_label_declaration();
    }

    // Handle other declaration types.
    switch (current_token_.type) {
        case TokenType::Manifest: return parse_manifest_declaration();
        case TokenType::Static:   return parse_static_declaration();
        case TokenType::Global:   return parse_global_declaration();

        // FUNCTION and ROUTINE keywords are handled by the main parse_program loop
        // to resolve LET ambiguity, so they are not parsed here.

        default:
            error("Unknown or unexpected declaration type.");
            return nullptr;
    }
}

/**
 * @brief Parses a label declaration of the form: LABEL: <statement>
 */
DeclPtr Parser::parse_label_declaration() {
    TraceGuard guard(*this, "parse_label_declaration");
    std::string name = current_token_.value;
    consume(TokenType::Identifier, "Expect identifier for label name.");
    consume(TokenType::Colon, "Expect ':' after label name.");
    auto command = parse_statement();
    return std::make_unique<LabelDeclaration>(name, std::move(command));
}

/**
 * @brief Parses a MANIFEST block: MANIFEST $( NAME = 123; ... $)
 */
DeclPtr Parser::parse_manifest_declaration() {
    TraceGuard guard(*this, "parse_manifest_declaration");
    consume(TokenType::Manifest, "Expect 'MANIFEST'.");
    consume(TokenType::LBrace, "Expect '$(' or '{' after MANIFEST.");

    // For simplicity, this parser handles only the first manifest constant.
    // A full implementation would loop until the closing brace.
    std::string name = current_token_.value;
    consume(TokenType::Identifier, "Expect identifier in manifest declaration.");
    consume(TokenType::Equal, "Expect '=' in manifest declaration.");

    // Note: A real compiler would need more robust error handling for stoll.
    long long value = std::stoll(current_token_.value);
    consume(TokenType::NumberLiteral, "Expect a number for manifest value.");

    // Skip other potential manifest constants in the block for this simple parser.
    while (!check(TokenType::RBrace) && !is_at_end()) {
        advance();
    }

    consume(TokenType::RBrace, "Expect '$)' or '}' to close MANIFEST block.");
    return std::make_unique<ManifestDeclaration>(name, value);
}

/**
 * @brief Parses a STATIC block: STATIC $( NAME = 123 $)
 */
DeclPtr Parser::parse_static_declaration() {
    TraceGuard guard(*this, "parse_static_declaration");
    consume(TokenType::Static, "Expect 'STATIC'.");
    consume(TokenType::LBrace, "Expect '$(' after STATIC.");

    std::string name = current_token_.value;
    consume(TokenType::Identifier, "Expect identifier in static declaration.");
    consume(TokenType::Equal, "Expect '=' in static declaration.");
    auto initializer = parse_expression();

    // Skip other potential static declarations.
    while (!check(TokenType::RBrace) && !is_at_end()) {
        advance();
    }

    consume(TokenType::RBrace, "Expect '$)' to close STATIC block.");
    return std::make_unique<StaticDeclaration>(name, std::move(initializer));
}

/**
 * @brief Parses a GLOBAL block: GLOBAL $( G1:0; G2:1 $)
 */
DeclPtr Parser::parse_global_declaration() {
    TraceGuard guard(*this, "parse_global_declaration");
    std::vector<std::pair<std::string, int>> globals;
    consume(TokenType::Global, "Expect 'GLOBAL'.");
    consume(TokenType::LBrace, "Expect '$(' after GLOBAL.");

    do {
        std::string name = current_token_.value;
        consume(TokenType::Identifier, "Expect identifier in global declaration.");
        consume(TokenType::Colon, "Expect ':' separating global name and offset.");
        int offset = std::stoi(current_token_.value);
        consume(TokenType::NumberLiteral, "Expect number for global offset.");
        globals.push_back({name, offset});

    } while (match(TokenType::Semicolon) || match(TokenType::Comma));

    consume(TokenType::RBrace, "Expect '$)' to close GLOBAL block.");
    return std::make_unique<GlobalDeclaration>(std::move(globals));
}

/**
 * @brief Parses a GET directive: GET "filename"
 */

