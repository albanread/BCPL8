#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(NumberLiteral& node) {
    debug_print("Visiting NumberLiteral node.");
    auto& register_manager = RegisterManager::getInstance();
    std::string dest_reg = register_manager.get_free_register(*this); // Get a free temporary register

    if (node.literal_type == NumberLiteral::LiteralType::Integer) {
        // Load the integer literal value into the destination register.
        // For small integers, use MOVZ directly.
        if (node.int_value >= 0 && node.int_value <= 0xFFFF) { // Fits in 16 bits
            emit(Encoder::create_movz_imm(dest_reg, static_cast<uint16_t>(node.int_value)));
            debug_print("Loaded integer literal " + std::to_string(node.int_value) + " into " + dest_reg + " using MOVZ.");
        } else {
            // For larger 64-bit integers, this might require multiple MOVZ/MOVK instructions.
            // The create_movz_movk_abs64 function handles this, but it should not set a target_label for literals.
            // The target_label argument should be empty for literal values.
            emit(Encoder::create_movz_movk_abs64(dest_reg, node.int_value, ""));
            debug_print("Loaded large integer literal " + std::to_string(node.int_value) + " into " + dest_reg + " using MOVZ/MOVK.");
        }
    } else { // Float literal
        // Use DataGenerator to register the float literal and get its label
        std::string float_label = data_generator_.add_float_literal(node.float_value);

        std::string temp_x_reg = register_manager.get_free_register(*this); // Need an X register for ADRP/ADD
        std::string dest_d_reg = register_manager.get_free_float_register(); // Get a free float register

        emit(Encoder::create_adrp(temp_x_reg, float_label));
        emit(Encoder::create_add_literal(temp_x_reg, temp_x_reg, float_label));
        emit(Encoder::create_ldr_fp_imm(dest_d_reg, temp_x_reg, 0)); // Load the double value

        register_manager.release_register(temp_x_reg); // Release the temporary X register
        dest_reg = dest_d_reg; // Set the result register to the float register
        debug_print("Loaded float literal " + std::to_string(node.float_value) + " into " + dest_d_reg + ".");
    }

    expression_result_reg_ = dest_reg; // Store the register holding the result
}