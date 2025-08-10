#include "../NewCodeGenerator.h"
#include "../LabelManager.h"
#include "../analysis/ASTAnalyzer.h"

// In NewCodeGenerator.cpp or gen_AssignmentStatement.cpp

void NewCodeGenerator::visit(AssignmentStatement& node) {
    debug_print("Visiting AssignmentStatement node.");

    if (node.lhs.size() != node.rhs.size()) {
        throw std::runtime_error("AssignmentStatement: Mismatch in number of LHS and RHS expressions.");
    }

    // Evaluate all RHS expressions first, storing results in temporary registers.
    std::vector<std::string> rhs_result_regs;
    for (const auto& rhs_expr : node.rhs) {
        generate_expression_code(*rhs_expr);
        rhs_result_regs.push_back(expression_result_reg_);
        // Do NOT release expression_result_reg_ yet.
    }

    // Now perform the assignments from RHS result registers to LHS locations.
    for (size_t i = 0; i < node.lhs.size(); ++i) {
        const auto& lhs_expr = node.lhs[i];
        const std::string& value_to_store_reg = rhs_result_regs[i];

        if (auto* var_access = dynamic_cast<VariableAccess*>(lhs_expr.get())) {
            handle_variable_assignment(var_access, value_to_store_reg);
        } else if (auto* vec_access = dynamic_cast<VectorAccess*>(lhs_expr.get())) {
            handle_vector_assignment(vec_access, value_to_store_reg);
        } else if (auto* char_indirection = dynamic_cast<CharIndirection*>(lhs_expr.get())) {
            handle_char_indirection_assignment(char_indirection, value_to_store_reg);
        } else if (auto* float_vec_indirection = dynamic_cast<FloatVectorIndirection*>(lhs_expr.get())) {
            handle_float_vector_indirection_assignment(float_vec_indirection, value_to_store_reg);
        } else {
            throw std::runtime_error("Unsupported LHS type for assignment.");
        }
    }

    debug_print("Finished visiting AssignmentStatement node.");
}

// Implementation for float vector indirection assignment
void NewCodeGenerator::handle_float_vector_indirection_assignment(FloatVectorIndirection* float_vec_indirection, const std::string& value_to_store_reg) {
    // Evaluate the vector base address
    generate_expression_code(*float_vec_indirection->vector_expr);
    std::string base_reg = expression_result_reg_;
    // Evaluate the index
    generate_expression_code(*float_vec_indirection->index_expr);
    std::string index_reg = expression_result_reg_;

    // Multiply index by 3 (since double is 8 bytes, float is 4 bytes so use 2 if float, 3 if double)
    emit(Encoder::create_lsl_imm(index_reg, index_reg, 3)); // LSL by 3 (multiply by 8 for double)

    // Add the offset to the base address to get the effective memory address
    std::string effective_addr_reg = register_manager_.get_free_register();
    emit(Encoder::create_add_reg(effective_addr_reg, base_reg, index_reg));
    register_manager_.release_register(base_reg);
    register_manager_.release_register(index_reg);

    // Store the 64-bit floating-point value from value_to_store_reg into the effective address
    emit(Encoder::create_str_fp_imm(value_to_store_reg, effective_addr_reg, 0));
    register_manager_.release_register(effective_addr_reg);
}
