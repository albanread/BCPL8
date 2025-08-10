#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(StringAllocationExpression& node) {
    debug_print("Visiting StringAllocationExpression node.");
    // `STRING size_expr`
    // Allocates a string (array of bytes) on the heap and returns its address.
    // Similar to `VEC`, but `size_expr` is already in bytes.

    // 1. Evaluate the size_expr (number of bytes).
    generate_expression_code(*node.size_expr);
    std::string size_bytes_reg = expression_result_reg_; // Register holding the number of bytes

    // 2. Move the size in bytes to X0 (first argument for `malloc`).
    emit(Encoder::create_mov_reg("X0", size_bytes_reg));
    auto& register_manager = register_manager_;
    register_manager.release_register(size_bytes_reg);

    // 3. Call the runtime `BCPL_ALLOC_CHARS` function.
    emit(Encoder::create_branch_with_link("BCPL_ALLOC_CHARS"));
    // `BCPL_ALLOC_CHARS` returns the allocated address in X0.

    // 4. The result of the allocation is the address in X0.
    expression_result_reg_ = "X0";
    register_manager_.mark_register_as_used("X0");

    debug_print("Finished visiting StringAllocationExpression node.");
}
