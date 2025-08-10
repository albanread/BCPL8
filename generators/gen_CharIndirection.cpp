#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"


void NewCodeGenerator::visit(CharIndirection& node) {
    debug_print("Visiting CharIndirection node.");
    auto& register_manager = register_manager_;
    // Char indirection like `string_expr % index_expr` or `string_expr ! index_expr` for characters.
    // This typically means `*(string_base + index_in_bytes)`.
    // Assuming `string_expr` evaluates to the base address (X register),
    // and `index_expr` evaluates to an integer index (X register).

    generate_expression_code(*node.string_expr);
    std::string string_base_reg = expression_result_reg_; // Holds the base address of the string

    generate_expression_code(*node.index_expr);
    std::string index_reg = expression_result_reg_; // Holds the index (in bytes)

    // Scale index by 4 for 32-bit character access
    emit(Encoder::create_lsl_imm(index_reg, index_reg, 2)); // index_reg <<= 2

    // Add the offset to the base address to get the effective memory address
    std::string effective_addr_reg = register_manager.get_free_register();
    emit(Encoder::create_add_reg(effective_addr_reg, string_base_reg, index_reg));
    register_manager.release_register(string_base_reg);
    register_manager.release_register(index_reg);

    // Load the 32-bit character value from the effective address into a W register
    std::string x_dest_reg = register_manager.get_free_register(); // Get X register
    std::string w_dest_reg = "W" + x_dest_reg.substr(1); // Convert "Xn" to "Wn"
    emit(Encoder::create_ldr_word_imm(w_dest_reg, effective_addr_reg, 0)); // Load 32-bit value
    register_manager.release_register(effective_addr_reg);

    expression_result_reg_ = w_dest_reg;
    debug_print("Finished visiting CharIndirection node.");
}
