#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(FloatVectorIndirection& node) {
    debug_print("Visiting FloatVectorIndirection node.");
    // Similar to VectorAccess, but loads a floating-point value (e.g., double).
    // Assumes `vector_expr` is base address (X register) and `index_expr` is index (X register).

    generate_expression_code(*node.vector_expr);
    std::string vector_base_reg = expression_result_reg_;

    generate_expression_code(*node.index_expr);
    std::string index_reg = expression_result_reg_;

    auto& register_manager = register_manager_;
    std::string dest_d_reg = register_manager.get_free_float_register(); // Destination is a float register

    // Calculate the byte offset: index * 8 (since double is 8 bytes)
    emit(Encoder::create_lsl_imm(index_reg, index_reg, 3)); // LSL by 3 (multiply by 8 for double)

    // Add the offset to the base address to get the effective memory address
    std::string effective_addr_reg = register_manager_.get_free_register(*this);
    emit(Encoder::create_add_reg(effective_addr_reg, vector_base_reg, index_reg));
    register_manager_.release_register(vector_base_reg);
    register_manager_.release_register(index_reg);

    // Load the 64-bit floating-point value from the effective address into a D register
    emit(Encoder::create_ldr_fp_imm(dest_d_reg, effective_addr_reg, 0));
    register_manager_.release_register(effective_addr_reg);

    expression_result_reg_ = dest_d_reg; // Result is in a float register
    debug_print("Finished visiting FloatVectorIndirection node.");
}