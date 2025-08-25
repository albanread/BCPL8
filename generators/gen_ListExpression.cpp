#include "../NewCodeGenerator.h"
#include "../RuntimeManager.h"
#include "../Encoder.h"
#include <vector>

namespace {
    // Helper to check if all expressions in a list are compile-time constants.
    bool are_all_initializers_constant(const std::vector<ExprPtr>& initializers) {
        for (const auto& expr : initializers) {
            if (!expr->is_literal()) {
                return false;
            }
        }
        return true;
    }
}

void NewCodeGenerator::visit(ListExpression& node) {
    debug_print("Visiting ListExpression node.");

    // First, check if we can use the efficient static template method.
    if (are_all_initializers_constant(node.initializers)) {
        debug_print("List is constant. Using static template generation.");
        // --- STATIC PATH (existing logic) ---
        std::string list_label = data_generator_.add_list_literal(&node);

        if (node.is_manifest) {
            std::string reg = register_manager_.get_free_register(*this);
            emit(Encoder::create_adrp(reg, list_label));
            emit(Encoder::create_add_literal(reg, reg, list_label));
            expression_result_reg_ = reg;
            register_manager_.mark_register_as_used(reg);
            debug_print("Emitted direct pointer load for MANIFESTLIST.");
        } else {
            emit(Encoder::create_adrp("X0", list_label));
            emit(Encoder::create_add_literal("X0", "X0", list_label));
            emit(Encoder::create_branch_with_link("DEEPCOPYLITERALLIST"));
            expression_result_reg_ = "X0";
            register_manager_.mark_register_as_used("X0");
            debug_print("Emitted deep copy logic for LIST.");
        }
    } else {
        debug_print("List contains live expressions. Using dynamic runtime construction.");
        // --- DYNAMIC PATH (new logic) ---

        // 1. Call the runtime to create a new, empty list header.
        emit(Encoder::create_branch_with_link("BCPL_LIST_CREATE_EMPTY"));
        // The pointer to the new header is now in X0.

        // 2. Store this pointer in a safe, persistent register.
        std::string list_header_reg = register_manager_.acquire_callee_saved_temp_reg(*current_frame_manager_);
        emit(Encoder::create_mov_reg(list_header_reg, "X0"));

        // 3. Iterate through each initializer expression and append it.
        for (const auto& expr : node.initializers) {
            // Evaluate the expression. Result is in expression_result_reg_
            generate_expression_code(*expr);
            std::string value_reg = expression_result_reg_;

            // Determine the type to call the correct append function.
            VarType expr_type = ASTAnalyzer::getInstance().infer_expression_type(expr.get());

            // Set up arguments for the append call.
            emit(Encoder::create_mov_reg("X0", list_header_reg)); // Arg 1: List header

            if (register_manager_.is_fp_register(value_reg)) {
                // Arg 2 for floats is in D1
                emit(Encoder::create_fmov_reg("D1", value_reg));
                emit(Encoder::create_branch_with_link("BCPL_LIST_APPEND_FLOAT"));
            } else {
                // Arg 2 for integers/pointers is in X1
                emit(Encoder::create_mov_reg("X1", value_reg));
                if (expr_type == VarType::POINTER_TO_STRING) {
                    emit(Encoder::create_branch_with_link("BCPL_LIST_APPEND_STRING"));
                } else if (expr_type == VarType::POINTER_TO_ANY_LIST ||
                           expr_type == VarType::POINTER_TO_INT_LIST ||
                           expr_type == VarType::POINTER_TO_FLOAT_LIST ||
                           expr_type == VarType::POINTER_TO_STRING_LIST) {
                    emit(Encoder::create_branch_with_link("BCPL_LIST_APPEND_LIST"));
                }
                else {
                    emit(Encoder::create_branch_with_link("BCPL_LIST_APPEND_INT"));
                }
            }
            register_manager_.release_register(value_reg);
        }

        // 4. The final result is the pointer to the list header.
        expression_result_reg_ = list_header_reg;
    }
}
