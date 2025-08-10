#include "../ASTAnalyzer.h"
#include <iostream>

/**
 * @brief Transforms the AST for semantic correctness.
 * Converts LET ... = VALOF ... constructs into standard FunctionDeclaration nodes.
 */
void ASTAnalyzer::transform(Program& program) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Starting AST transformation..." << std::endl;
    transform_let_declarations(program.declarations);
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] AST transformation complete." << std::endl;
}