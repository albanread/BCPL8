#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(VectorAccess& node) {
    debug_print("Visiting VectorAccess node.");
    // Vector access like `vector_expr % index_expr` or `vector_expr ! index_expr`
    // In BCPL, this often means `*(vector_base + index_in_words * 8)`.
    // Assuming `vector_expr` evaluates to the base address (X register),
    // and `index_expr` evaluates to an integer index (X register).

    generate_expression_code(*node.vector_expr);
    std::string vector_base_reg = expression_result_reg_; // Holds the base address of the vector

    generate_expression_code(*node.index_expr);
    std::string index_reg = expression_result_reg_; // Holds the index

    auto& register_manager = register_manager_;
    std::string dest_reg = register_manager.get_free_register(); // Register for the loaded value

    // Calculate the byte offset: index * 8 (since BCPL words are 8 bytes)
    emit(Encoder::create_lsl_imm(index_reg, index_reg, 3)); // LSL by 3 (left shift by 3 is multiply by 8)
    debug_print("Calculated byte offset for vector access.");

    // Add the offset to the base address to get the effective memory address
    // ADD Xeff_addr, vector_base_reg, index_reg
    std::string effective_addr_reg = register_manager_.get_free_register();
    emit(Encoder::create_add_reg(effective_addr_reg, vector_base_reg, index_reg));
    register_manager.release_register(vector_base_reg);
    register_manager.release_register(index_reg);

    // Load the 64-bit value from the effective address
    emit(Encoder::create_ldr_imm(dest_reg, effective_addr_reg, 0)); // Load from [effective_addr_reg + 0]
    register_manager.release_register(effective_addr_reg);

    expression_result_reg_ = dest_reg;
    debug_print("Finished visiting VectorAccess node.");
}