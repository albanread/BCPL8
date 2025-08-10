#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(TableExpression& node) {
    debug_print("Visiting TableExpression node.");
    // `TABLE expr, expr, ...`
    // This typically creates a constant array in the data section and returns its address.

    // 1. Create a unique label for this table in the data section.
    std::string table_label = label_manager_.create_label();

    // 2. Define the label in the instruction stream (data section).
    instruction_stream_.define_label(table_label);

    // 3. Emit each initializer as a 64-bit data word.
    for (const auto& init_expr : node.initializers) {
        // For table initializers, they must typically be constant expressions that can be
        // evaluated at compile time. We'll simplify and only support NumberLiterals.
        if (init_expr->is_literal()) {
            if (NumberLiteral* num_lit = dynamic_cast<NumberLiteral*>(init_expr.get())) {
                if (num_lit->literal_type == NumberLiteral::LiteralType::Integer) {
                    instruction_stream_.add_data64(num_lit->int_value);
                    debug_print("Emitted table entry: " + std::to_string(num_lit->int_value) + ".");
                } else {
                    throw std::runtime_error("TableExpression: Float literals not yet supported as table initializers.");
                }
            } else {
                throw std::runtime_error("TableExpression: Non-numeric literal table initializers not yet supported.");
            }
        } else {
            throw std::runtime_error("TableExpression: Non-literal initializers not supported (must be constant).");
        }
    }
    // Ensure 8-byte alignment after the table data if necessary (for subsequent data or code)
    instruction_stream_.add_data_padding(8);


    // 4. Load the address of this table into the destination register.
    auto& register_manager = register_manager_;
    std::string dest_reg = register_manager.get_free_register();
    emit(Encoder::create_adrp(dest_reg, table_label));
    emit(Encoder::create_add_literal(dest_reg, dest_reg, table_label));

    expression_result_reg_ = dest_reg; // The register now holds the memory address of the table
    debug_print("Finished visiting TableExpression node, address loaded into " + dest_reg + ".");
}