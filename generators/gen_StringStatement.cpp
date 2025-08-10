#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "RegisterManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(StringStatement& node) {
    debug_print("Visiting StringStatement node.");
    // `STRING size_expr` as a statement.
    // In BCPL, `STRING` can be an expression or a statement.
    // As a statement, it usually implies allocating and perhaps initializing a string
    // but not necessarily using its return value immediately.
    // For simplicity, we'll treat it similar to the expression version, but the
    // result register might not be explicitly kept.

    // 1. Evaluate the size_expr (number of bytes).
    generate_expression_code(*node.size_expr);
    std::string size_bytes_reg = expression_result_reg_;

    // 2. Move size to X0.
    emit(Encoder::create_mov_reg("X0", size_bytes_reg));
    auto& register_manager = register_manager_;
    register_manager.release_register(size_bytes_reg);

    // 3. Call BCPL_ALLOC_CHARS.
    emit(Encoder::create_branch_with_link("BCPL_ALLOC_CHARS"));

    // The allocated address is in X0, but since it's a statement, it might not be used.
    // X0 is implicitly clobbered by the call, so no explicit release needed unless we want to preserve it.
    debug_print("Generated code for StringStatement (allocating string memory).");
}