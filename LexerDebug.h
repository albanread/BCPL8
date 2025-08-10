#ifndef LEXER_DEBUG_H
#define LEXER_DEBUG_H

#include <string>
#include <iostream>

extern bool g_enable_lexer_trace;

/**
 * @brief A simple utility for printing trace messages during lexing and parsing.
 *
 * This function provides a single, consistent way to output debug information,
 * abstracting the direct use of std::cout. It can be easily modified to redirect
 * output to a file or a different stream if needed.
 *
 * @param message The debug message to print.
 */
inline void LexerTrace(const std::string& message) {
    if (!g_enable_lexer_trace) return; // Only print if tracing is enabled
    // This can be replaced with a more sophisticated logging framework later.
    // For now, it prints directly to standard error to separate debug output
    // from potential program output.
    std::cerr << "[TRACE] " << message << std::endl;
}

#endif // LEXER_DEBUG_H
