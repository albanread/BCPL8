#include "../NewCodeGenerator.h"
#include "../DataGenerator.h"
#include "../Encoder.h"
#include "../RegisterManager.h"
#include "../AST.h"
#include <stdexcept>

void NewCodeGenerator::visit(TableExpression& node) {
    debug_print("Visiting TableExpression node.");

    // 1. Register the table literal with the DataGenerator.
    // This will create the data in the .rodata section and return a label for it.
    std::string table_label;
    if (node.is_float_table) {
        table_label = data_generator_.add_float_table_literal(node.initializers);
    } else {
        table_label = data_generator_.add_table_literal(node.initializers);
    }

    // 2. Acquire a register to hold the address of the table.
    std::string dest_reg = register_manager_.get_free_register(*this);

    // 3. Emit the standard ADRP + ADD sequence to load the address of the label
    // into the destination register.
    emit(Encoder::create_adrp(dest_reg, table_label));
    emit(Encoder::create_add_literal(dest_reg, dest_reg, table_label));

    // 4. Add 8 to the pointer to skip the 64-bit length prefix.
    //    The register now points to the payload, matching the VEC convention.
    emit(Encoder::create_add_imm(dest_reg, dest_reg, 8));

    // 5. The result of this expression is the register holding the address of the table's payload.
    expression_result_reg_ = dest_reg;

    debug_print("Finished visiting TableExpression. Address for '" + table_label + "' is in " + dest_reg);
}