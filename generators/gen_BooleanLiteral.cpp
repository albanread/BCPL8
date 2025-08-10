#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(BooleanLiteral& node) {
    debug_print("Visiting BooleanLiteral node.");
    auto& register_manager = register_manager_;
    std::string dest_reg = register_manager.get_free_register(); // Get a free temporary register

    // BCPL booleans are typically 0 for FALSE and 1 for TRUE.
    int64_t bool_val = node.value ? 1 : 0;
    emit(Encoder::create_movz_imm(dest_reg, static_cast<int>(bool_val), 0));
    expression_result_reg_ = dest_reg;
    debug_print("Loaded boolean literal " + std::string(node.value ? "TRUE" : "FALSE") + " into " + dest_reg + ".");
}