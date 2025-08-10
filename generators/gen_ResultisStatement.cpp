#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(ResultisStatement& node) {
    debug_print("Visiting ResultisStatement node.");

    // 1. Evaluate the expression. The result is now live in expression_result_reg_.
    generate_expression_code(*node.expression);
    std::string result_reg = expression_result_reg_;

    // 2. The ABI requires the return value to be in X0 for integers or D0 for floats.
    // First, check if we're expecting a floating-point result from the analyzer
    bool should_be_float = false;
    
    // If we're in a FloatValofExpression, the return type is float
    if (current_function_return_type_ == VarType::FLOAT) {
        should_be_float = true;
    } else {
        // Otherwise, check the expression type
        if (auto* num_lit = dynamic_cast<NumberLiteral*>(node.expression.get())) {
            if (num_lit->literal_type == NumberLiteral::LiteralType::Float) {
                should_be_float = true;
            }
        }
    }
    
    if (should_be_float || register_manager_.is_fp_register(result_reg)) {
        // --- Float Result ---
        // If the result isn't in a float register but should be, convert it
        if (!register_manager_.is_fp_register(result_reg) && should_be_float) {
            std::string fp_reg = register_manager_.acquire_fp_scratch_reg();
            emit(Encoder::create_scvtf_reg(fp_reg, result_reg));
            register_manager_.release_register(result_reg);
            result_reg = fp_reg;
        }
        
        // Now move to D0 if not already there
        if (result_reg != "D0") {
            emit(Encoder::create_fmov_reg("D0", result_reg));
            register_manager_.release_register(result_reg);
        }
        // The final result is now in D0.
        expression_result_reg_ = "D0";
    } else {
        // --- Integer Result ---
        // If the result is in a float register but shouldn't be, convert it
        if (register_manager_.is_fp_register(result_reg) && !should_be_float) {
            std::string int_reg = register_manager_.acquire_scratch_reg();
            emit(Encoder::create_fcvtzs_reg(int_reg, result_reg));
            register_manager_.release_register(result_reg);
            result_reg = int_reg;
        }
        
        // Now move to X0 if not already there
        if (result_reg != "X0") {
            emit(Encoder::create_mov_reg("X0", result_reg));
            register_manager_.release_register(result_reg);
        }
        // The final result is now in X0.
        expression_result_reg_ = "X0";
    }

    // 3. Branch to the function's single, shared epilogue.
    emit(Encoder::create_branch_unconditional(current_function_epilogue_label_));
    debug_print("RESULTIS: Branching to function epilogue label " + current_function_epilogue_label_);
}