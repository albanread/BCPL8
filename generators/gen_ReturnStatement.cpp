#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(ReturnStatement& node) {
    debug_print("Visiting ReturnStatement node.");
    // `RETURN` (implicitly returns current value of X0/D0 or undefined)
    // Or it might be for a `ROUTINE` (which doesn't return a value).

    if (current_frame_manager_) {
        // Branch to the function's shared epilogue label.
        emit(Encoder::create_branch_unconditional(current_function_epilogue_label_));
        debug_print("RETURN: Branching to function epilogue label " + current_function_epilogue_label_);
    } else {
        // This RETURN is not within a function/routine (e.g., top-level).
        // This is an error or implies exiting the program.
        // For BCPL, top-level might just implicitly `FINISH`.
        throw std::runtime_error("RETURN statement outside of a function or routine context.");
    }
}