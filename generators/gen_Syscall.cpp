#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include <iostream>
#include <stdexcept>

/**
 * @brief Generates ARM64 assembly code for a system call on macOS.
 *
 * This function follows the Apple ARM64 ABI for system calls:
 * 1. The syscall number is placed in register X16.
 * 2. Arguments are placed in registers X0, X1, X2, and so on.
 * 3. The syscall is invoked with the `SVC #0x80` instruction.
 * 4. The return value is placed in register X0.
 *
 * @param node The SysCall AST node to visit.
 */
void NewCodeGenerator::visit(SysCall& node) {
    debug_print("Visiting SysCall node for macOS.");

    // --- STEP 1: Evaluate and Place Syscall Arguments (X0, X1, ...) ---
    int arg_count = 0;
    for (const auto& arg_expr : node.arguments) {
        // Generate code to evaluate the argument expression. The result will be in expression_result_reg_.
        generate_expression_code(*arg_expr);
        std::string arg_val_reg = expression_result_reg_;

        // Argument registers on ARM64 are X0, X1, X2, ...
        if (arg_count < 6) { // A common limit before stack usage.
            std::string dest_arg_reg = "X" + std::to_string(arg_count);
            // If the argument's value is not already in the correct register, move it.
            if (arg_val_reg != dest_arg_reg) {
                emit(Encoder::create_mov_reg(dest_arg_reg, arg_val_reg));
                // If the source was a temporary register, release it.
                if (register_manager_.is_scratch_register(arg_val_reg)) {
                    register_manager_.release_scratch_reg(arg_val_reg);
                }
            }
        } else {
            // Stack-based arguments for syscalls are complex and not implemented here.
            throw std::runtime_error("SysCall: More than 6 arguments are not yet implemented.");
        }
        arg_count++;
    }

    // --- STEP 2: Place the Syscall Number in X16 ---
    // The syscall number itself is an expression that needs to be evaluated.
    debug_print("Evaluating syscall number expression.");
    generate_expression_code(*node.syscall_number);
    std::string syscall_num_reg = expression_result_reg_;

    // The syscall number must be in X16 for the `SVC` instruction on macOS.
    debug_print("Moving syscall number to X16.");
    emit(Encoder::create_mov_reg("X16", syscall_num_reg));

    // Release the register used for the syscall number if it was a temporary one.
    if (register_manager_.is_scratch_register(syscall_num_reg)) {
        register_manager_.release_scratch_reg(syscall_num_reg);
    }

    // --- STEP 3: Emit the Syscall Instruction ---
    // The `SVC #0x80` instruction triggers the system call on XNU-based kernels (like macOS).
    debug_print("Emitting SVC #0x80 instruction.");
    emit(Encoder::create_svc_imm(0x80));

    // --- STEP 4: Handle the Return Value ---
    // The result of a system call is returned in X0 by convention.
    debug_print("Syscall result is in X0.");
    expression_result_reg_ = "X0";
    register_manager_.mark_register_as_used("X0");
}
