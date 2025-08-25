#include "../NewCodeGenerator.h"

#include <stdexcept>

void NewCodeGenerator::visit(FVecAllocationExpression& node) {
    debug_print("Visiting FVecAllocationExpression node.");
    // `FVEC size_expr`
    // This allocates a vector (array) of float words on the heap and returns its address.
    // This typically translates to a call to a runtime memory allocation routine,
    // identical to VecAllocationExpression, but tracked as a float vector.

    // 1. Evaluate the size_expr (number of words).
    generate_expression_code(*node.size_expr);
    std::string size_words_reg = expression_result_reg_; // Register holding the number of words

    // 2. Move the number of words directly to X0 (BCPL_ALLOC_WORDS expects words, not bytes).
    auto& register_manager = register_manager_;
    emit(Encoder::create_mov_reg("X0", size_words_reg));
    register_manager.release_register(size_words_reg);

    // 3. Load the ADDRESS of the function name string into X1.
    std::string func_name_label = data_generator_.add_string_literal(current_frame_manager_->get_function_name());
    emit(Encoder::create_adrp("X1", func_name_label));
    emit(Encoder::create_add_literal("X1", "X1", func_name_label));
    // Do NOT release X1, it's an argument for the upcoming call.

    // 4. Load the ADDRESS of the variable name string into X2.
    std::string var_name_label = data_generator_.add_string_literal(node.get_variable_name());
    emit(Encoder::create_adrp("X2", var_name_label));
    emit(Encoder::create_add_literal("X2", "X2", var_name_label));
    // Do NOT release X2, it's an argument for the upcoming call.

    // 5. Call the runtime `BCPL_ALLOC_WORDS` function using the X28-relative pointer table.
    size_t offset = RuntimeManager::instance().get_function_offset("BCPL_ALLOC_WORDS");
    std::string addr_reg = register_manager_.acquire_scratch_reg(*this);
    Instruction ldr_instr = Encoder::create_ldr_imm(addr_reg, "X19", offset);
    ldr_instr.jit_attribute = JITAttribute::JitAddress;
    emit(ldr_instr);
    Instruction blr_instr = Encoder::create_branch_with_link_register(addr_reg);
    blr_instr.jit_attribute = JITAttribute::JitCall;
    emit(blr_instr);
    register_manager.release_register(addr_reg);

    // 6. The result (pointer to float vector) is now in X0.
    expression_result_reg_ = "X0";
}