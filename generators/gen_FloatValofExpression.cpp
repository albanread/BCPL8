#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(FloatValofExpression& node) {
    debug_print("Visiting FloatValofExpression node.");
    // `FVALOF <stmt> RESULTIS expr`
    // This creates a block that evaluates to a floating point value.

    // Enter a new scope for the FVALOF block.
    // A FVALOF block is part of the parent function's scope.

    // The result of the FloatValofExpression will be determined by the `RESULTIS` statement.
    // The `RESULTIS` statement will set `expression_result_reg_` to a floating point register.

    // Mark that we're processing a floating-point expression for ResultisStatement
    current_function_return_type_ = VarType::FLOAT;
    
    generate_statement_code(*node.body);

    // Exit the scope of the FVALOF block.
    // Exiting FVALOF block processing.

    // Ensure we are using a floating-point register (D0) for the result
    if (!register_manager_.is_fp_register(expression_result_reg_)) {
        debug_print("WARNING: Expression result register is not a floating-point register. Converting to D0.");
        std::string fp_reg = register_manager_.acquire_fp_scratch_reg();
        emit(Encoder::create_scvtf_reg(fp_reg, expression_result_reg_));
        register_manager_.release_register(expression_result_reg_);
        expression_result_reg_ = fp_reg;
    }

    // `expression_result_reg_` should now contain the floating point value from the `RESULTIS` statement.
    debug_print("Finished visiting FloatValofExpression node. Result in register: " + expression_result_reg_);
}