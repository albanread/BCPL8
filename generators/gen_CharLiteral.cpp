#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(CharLiteral& node) {
    debug_print("Visiting CharLiteral node.");
    auto& register_manager = register_manager_;
    std::string x_dest_reg = register_manager.get_free_register(*this); // Get a free X register
    std::string w_dest_reg = "W" + x_dest_reg.substr(1); // Convert "Xn" to "Wn"

    // Load the character's value into a 32-bit register (Wn)
    emit(Encoder::create_movz_imm(w_dest_reg, static_cast<int>(node.value), 0));
    
    // --- START OF FIX ---
    // Return the 64-bit X register. Writing to the W register automatically
    // zero-extends the upper 32 bits of the corresponding X register.
    expression_result_reg_ = x_dest_reg; 
    // --- END OF FIX ---

    debug_print("Loaded char literal '" + std::string(1, node.value) + "' (ASCII: " + std::to_string(static_cast<int>(node.value)) + ") into " + w_dest_reg + ", result in " + x_dest_reg + ".");
}