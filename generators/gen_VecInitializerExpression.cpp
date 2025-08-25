#include "NewCodeGenerator.h"
#include "AST.h"
#include "Encoder.h"
#include <iostream>

void NewCodeGenerator::visit(VecInitializerExpression& node) {
    debug_print("Visiting VecInitializerExpression node.");
    size_t num_initializers = node.initializers.size();

    // 1. Allocate the vector on the heap by calling the runtime.
    // The size goes in X0.
    emit(Encoder::create_movz_movk_abs64("X0", num_initializers, ""));
    emit(Encoder::create_branch_with_link("BCPL_ALLOC_WORDS"));
    // The pointer to the new vector is now in X0.

    // 2. Acquire a spillable temp register to preserve the vector pointer.
    std::string vec_ptr_reg = register_manager_.acquire_spillable_temp_reg(*this);
    emit(Encoder::create_mov_reg(vec_ptr_reg, "X0"));

    // If there's nothing to initialize, we're done.
    if (num_initializers == 0) {
        expression_result_reg_ = vec_ptr_reg;
        return;
    }

    // 3. Evaluate each expression and store its result in the vector.
    for (size_t i = 0; i < num_initializers; ++i) {
        // Evaluate the initializer expression.
        generate_expression_code(*node.initializers[i]);
        std::string value_reg = expression_result_reg_;

        // Calculate the address of the vector element: vec_ptr + (i * 8)
        std::string offset_reg = register_manager_.acquire_scratch_reg(*this);
        std::string element_addr_reg = register_manager_.acquire_scratch_reg(*this);

        emit(Encoder::create_movz_movk_abs64(offset_reg, i * 8, ""));
        emit(Encoder::create_add_reg(element_addr_reg, vec_ptr_reg, offset_reg));
        
        // Store the value. Check if it's a float or integer.
        if (register_manager_.is_fp_register(value_reg)) {
            emit(Encoder::create_str_fp_imm(value_reg, element_addr_reg, 0));
        } else {
            emit(Encoder::create_str_imm(value_reg, element_addr_reg, 0));
        }

        // Clean up registers for the next iteration.
        register_manager_.release_register(value_reg);
        register_manager_.release_register(offset_reg);
        register_manager_.release_register(element_addr_reg);
    }

    // 4. The final result of this expression is the preserved pointer register.
    expression_result_reg_ = vec_ptr_reg;
}