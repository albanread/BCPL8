#include "../NewCodeGenerator.h"
#include "../analysis/ASTAnalyzer.h"
#include "../RuntimeManager.h"
#include "../Symbol.h"
#include <stdexcept>
#include <vector>
#include <string>
#include <sstream> // For building log messages
#include <cstdint> // For uint64_t
#include "CodeGenUtils.h"

// Assuming Encoder, RegisterManager, ASTNode, VariableAccess, FunctionType, Expression etc. are defined elsewhere



void NewCodeGenerator::visit(RoutineCallStatement& node) {
    debug_print("--- Entering NewCodeGenerator::visit(RoutineCallStatement& node) ---");
    // FIX 1: Cast to ASTNode* to ensure toString() is available
    debug_print("Processing RoutineCallStatement for routine: type=" + std::to_string(static_cast<int>(node.routine_expr->getType())));

    // Get routine name if available through a VariableAccess
    std::string routine_name;
    if (auto* var_access = dynamic_cast<VariableAccess*>(node.routine_expr.get())) {
        routine_name = var_access->name;
    }

    // Check if this is a runtime routine in our symbol table
    Symbol symbol;
    bool has_symbol = false;
    if (!routine_name.empty()) {
        has_symbol = lookup_symbol(routine_name, symbol);
    }

    // Local helper function to save caller-saved registers
    // Now returns the vector of saved registers and handles stack alignment.
    auto saveCallerSavedRegisters = [&](std::vector<std::string>& saved_regs) -> bool {
        debug_print("Saving caller-saved registers...");
        // Use the specialized method to reset only caller-saved registers
        // without affecting the routine address cache in X19/X20
        register_manager_.reset_caller_saved_registers();
        saved_regs = register_manager_.get_in_use_caller_saved_registers(); // Get and store the exact list
        int register_count = saved_regs.size();
        bool align_stack = (register_count % 2 != 0); // Need to align stack if odd number of regs

        std::stringstream saved_regs_ss;
        saved_regs_ss << "Registers to save: [";
        for (size_t i = 0; i < saved_regs.size(); ++i) {
            saved_regs_ss << saved_regs[i] << (i == saved_regs.size() - 1 ? "" : ", ");
        }
        saved_regs_ss << "]";
        debug_print(saved_regs_ss.str());

        // Store pairs of registers using STP (Store Pair) instruction with pre-indexed addressing
        for (size_t i = 0; i < saved_regs.size(); i += 2) {
            std::string reg1 = saved_regs[i];
            std::string reg2 = (i + 1 < saved_regs.size()) ? saved_regs[i + 1] : "ZR"; // Pair with ZR if odd number
            // FIX 2a: Reverted to create_stp_pre_imm as per your Encoder.h
            emit(Encoder::create_stp_pre_imm(reg1, reg2, "SP", -16));
            debug_print("Emitting STP " + reg1 + ", " + reg2 + ", [SP, #-16]!");
        }

        // Apply final 16-byte alignment if an odd number of actual 8-byte registers were pushed
        // (i.e., if the last STP used ZR and the stack wasn't already 16-byte aligned by the STPs themselves)
        // This is necessary if the total bytes pushed by STPs (which are always multiples of 16)
        // still leaves the stack unaligned relative to the *initial* SP if it was not 16-byte aligned.
        // Or, more simply, if the _count_ of actual registers is odd, an extra 16 bytes are effectively consumed.
        if (align_stack) {
            emit(Encoder::create_sub_imm("SP", "SP", 16));
            debug_print("Emitting SUB SP, SP, #16 (stack alignment for odd register count).");
        }
        debug_print("Finished saving caller-saved registers. Stack aligned: " + std::string(align_stack ? "true" : "false"));
        return align_stack;
    };

    // Local helper function to restore caller-saved registers
    // FIX 3: Removed duplicate definition and fixed signature.
    // FIX 4: Changed signature to take only the saved_regs vector.
    auto restoreCallerSavedRegisters = [&](bool align_stack_needed_from_save, const std::vector<std::string>& saved_regs) {
        debug_print("Restoring caller-saved registers...");

        // Restore stack alignment if it was adjusted during saving
        if (align_stack_needed_from_save) {
            emit(Encoder::create_add_imm("SP", "SP", 16));
            debug_print("Emitting ADD SP, SP, #16 (restoring stack alignment).");
        }

        std::stringstream restored_regs_ss;
        restored_regs_ss << "Registers to restore: [";
        // Iterate backwards through the saved_regs list to restore in reverse order
        for (int i = saved_regs.size() - 1; i >= 0; --i) {
            restored_regs_ss << saved_regs[i] << (i == 0 ? "" : ", ");
        }
        restored_regs_ss << "]";
        debug_print(restored_regs_ss.str());

        // Restore pairs of registers using LDP (Load Pair) instruction with post-indexed addressing
        // Iterate backwards by 2 to match STP pushes.
        for (int i = saved_regs.size() - 1; i >= 0; i -= 2) {
            // Ensure we have at least two registers for LDP, or handle the single one if list size is odd.
            // This robustly handles the case where saved_regs.size() is 1 (e.g., [X10])
            std::string reg1;
            std::string reg2;

            if (i == 0 && saved_regs.size() % 2 != 0) { // If only one register left and original count was odd
                reg1 = saved_regs[0]; // The single remaining register
                reg2 = "ZR"; // Paired with ZR during STP, so we load 'reg1' and effectively discard ZR from the pair slot
            } else {
                // Ensure i-1 is valid for pairing
                if (i - 1 < 0) {
                     throw std::runtime_error("Error: Invalid register index during LDP restoration loop.");
                }
                reg1 = saved_regs[i - 1];
                reg2 = saved_regs[i];
            }
            // FIX 2b: Reverted to create_ldp_post_imm as per your Encoder.h
            emit(Encoder::create_ldp_post_imm(reg1, reg2, "SP", 16));
            debug_print("Emitting LDP " + reg1 + ", " + reg2 + ", [SP], #16");
        }
        debug_print("Finished restoring caller-saved registers.");
    };


    // Local helper function to evaluate arguments and store them in safe, temporary registers.
    auto evaluateArguments = [&](std::vector<std::string>& arg_result_regs) {
        debug_print("Evaluating routine arguments...");
        for (size_t arg_idx = 0; arg_idx < node.arguments.size(); ++arg_idx) {
            const auto& arg_expr = node.arguments[arg_idx];
            debug_print("  Evaluating Argument " + std::to_string(arg_idx) + ": type=" + std::to_string(static_cast<int>(arg_expr->getType())));

            generate_expression_code(*arg_expr); // Result is in expression_result_reg_
            debug_print("    Argument " + std::to_string(arg_idx) + " result in: " + expression_result_reg_);

            // The argument's value is now in expression_result_reg_.
            // Instead of acquiring a new register and moving the value,
            // we will "hold" this register until the arguments are moved
            // into their final ABI positions (X0-X7 or D0-D7).
            arg_result_regs.push_back(expression_result_reg_);

            // Do NOT release expression_result_reg_ here. It is now tracked in the
            // arg_result_regs vector and will be released later.
            debug_print("    Holding result from " + expression_result_reg_ + " for argument setup.");
        }
        debug_print("Finished evaluating arguments.");
    };

    // Local helper function to determine if the target is a float function based on its name.
    auto isFloatCallTarget = [&](const RoutineCallStatement& call_node) -> bool {
        debug_print("Determining if call target is a float function...");
        if (auto* var_access = dynamic_cast<VariableAccess*>(call_node.routine_expr.get())) {
            const std::string& func_name = var_access->name;
            debug_print("  Call target is VariableAccess: " + func_name);
            
            // First check in symbol table if available
            if (symbol_table_) {
                Symbol func_symbol;
                if (symbol_table_->lookup(func_name, func_symbol)) {
                    bool is_float = func_symbol.is_float_function();
                    debug_print("  Target '" + func_name + "' identified as " + 
                               (is_float ? "FLOAT" : "INTEGER") + " function by symbol table.");
                    return is_float;
                }
            }
            
            // Fall back to ASTAnalyzer if not in symbol table
            auto& return_types = ASTAnalyzer::getInstance().get_function_return_types();
            if (return_types.find(func_name) != return_types.end()) {
                debug_print("  Function '" + func_name + "' found in AST analyzer return types map");
                if (return_types.at(func_name) == VarType::FLOAT) {
                    debug_print("  Target '" + func_name + "' identified as FLOAT function by AST analyzer.");
                    return true;
                }
                debug_print("  Target '" + func_name + "' identified as INTEGER function by AST analyzer.");
            } else if (RuntimeManager::instance().is_function_registered(func_name)) {
                // Check runtime functions if not found in AST analyzer
                FunctionType func_type = RuntimeManager::instance().get_function(func_name).type;
                if (func_type == FunctionType::FLOAT) {
                    debug_print("  Target '" + func_name + "' identified as RUNTIME FLOAT function.");
                    return true;
                }
                debug_print("  Target '" + func_name + "' identified as RUNTIME INTEGER function.");
            } else {
                debug_print("  Function '" + func_name + "' not found in any type registry - assuming INTEGER function.");
            }
        }
        debug_print("  Call target not identified as a float function (or not a direct variable access).");
        return false;
    };

    // Local helper function to move arguments from temporary registers to final argument registers (X0-X7 or D0-D7).
    auto moveArgumentsToCallRegisters = [&](const std::vector<std::string>& arg_result_regs, bool is_float_call) {
        debug_print("Moving arguments to call registers (X0-X7 or D0-D7)...");
        debug_print("  Call is " + std::string(is_float_call ? "FLOAT" : "INTEGER") + " type.");

        for (size_t i = 0; i < arg_result_regs.size(); ++i) {
            std::string src_reg = arg_result_regs[i];
            debug_print("  Processing Argument " + std::to_string(i) + " from temporary register: " + src_reg);

            if (is_float_call) {
                std::string dest_d_reg = "D" + std::to_string(i);
                if (register_manager_.is_fp_register(src_reg)) {
                    if (src_reg != dest_d_reg) {
                        debug_print("    FMOV: " + dest_d_reg + " <- " + src_reg + " (FP to FP direct move)");
                        emit(Encoder::create_fmov_reg(dest_d_reg, src_reg));
                    } else {
                        debug_print("    Argument " + std::to_string(i) + " already in " + src_reg + " (D-register). No FMOV needed.");
                    }
                } else {
                    debug_print("    SCVTF: " + dest_d_reg + " <- " + src_reg + " (GP to FP conversion)");
                    emit(Encoder::create_scvtf_reg(dest_d_reg, src_reg));
                }
                register_manager_.mark_register_as_used(dest_d_reg);
                debug_print("    Marked " + dest_d_reg + " as used.");
            } else { // Not a float call, handle as a standard integer/pointer argument
                std::string dest_x_reg = "X" + std::to_string(i);
                if (register_manager_.is_fp_register(src_reg)) { // Float -> Integer conversion needed
                    debug_print("    Converting float in " + src_reg + " to integer in " + dest_x_reg + " (float to int)");
                    generate_float_to_int_truncation(dest_x_reg, src_reg);
                } else { // Integer -> Integer (simple register move)
                    if (src_reg != dest_x_reg) {
                        debug_print("    Moving from " + src_reg + " to " + dest_x_reg + " (int to int)");
                        emit(Encoder::create_mov_reg(dest_x_reg, src_reg));
                    } else {
                        debug_print("    No move needed; source already in correct X register: " + src_reg);
                    }
                }
                register_manager_.mark_register_as_used(dest_x_reg);
                debug_print("    Marked " + dest_x_reg + " as used.");
            }
            register_manager_.release_register(src_reg);
            debug_print("  Released temporary register: " + src_reg);
        }
        debug_print("Finished moving arguments to call registers.");
    };

    // Local helper function for handling runtime function calls (X28-relative table).
    auto handleRuntimeCall = [&](VariableAccess* var_access) {
        debug_print("Handling runtime routine call (X28-relative): " + var_access->name);

        // 1. Get the routine's pre-assigned table offset from the RuntimeManager.
        size_t offset = RuntimeManager::instance().get_function_offset(var_access->name);

        // 2. Acquire a temporary scratch register to hold the function address.
        std::string addr_reg = register_manager_.acquire_scratch_reg(*this);

        // 3. Emit the LDR instruction to load the pointer from the global table.
        Instruction ldr_instr = Encoder::create_ldr_imm(addr_reg, "X19", offset);
        ldr_instr.jit_attribute = JITAttribute::JitAddress;
        emit(ldr_instr);

        // 4. Emit the indirect branch.
        Instruction blr_instr = Encoder::create_branch_with_link_register(addr_reg);
        blr_instr.jit_attribute = JITAttribute::JitCall;
        blr_instr.target_label = var_access->name;
        blr_instr.assembly_text += " ; call " + var_access->name;
        emit(blr_instr);

        // 5. Release the scratch register.
        register_manager_.release_register(addr_reg);
        debug_print("Generated LDR/BLR sequence for runtime call to '" + var_access->name + "'");
    };

    // Local helper function for handling general routine calls (user-defined or function pointers).
    auto handleGeneralRoutineCall = [&](Expression* routine_expr_ptr) {
        debug_print("Handling general routine call (user-defined or function pointer).");
        if (!routine_expr_ptr) {
            throw std::runtime_error("Error: Null routine expression pointer in handleGeneralRoutineCall.");
        }
        Expression& routine_expr_ref = *routine_expr_ptr;

        generate_expression_code(routine_expr_ref);
        std::string routine_addr_reg = expression_result_reg_;
        debug_print("  Routine address evaluated into register: " + routine_addr_reg);
        Instruction blr_instr = Encoder::create_branch_with_link_register(routine_addr_reg);
        blr_instr.jit_attribute = JITAttribute::JitCall;
        emit(blr_instr);
        debug_print("  Generated BLR call using address in " + routine_addr_reg + ".");
        register_manager_.release_register(routine_addr_reg);
        debug_print("  Released routine address register: " + routine_addr_reg);
    };


    // --- Main Logic for RoutineCallStatement ---

    // Variable to hold the list of registers actually saved
    std::vector<std::string> actual_saved_caller_saved_regs;
    bool align_stack_needed_after_save = false; // Flag returned by save, passed to restore

    // 1. Save in-use caller-saved registers to the stack
    align_stack_needed_after_save = saveCallerSavedRegisters(actual_saved_caller_saved_regs);


    // 2. Evaluate all arguments first and store results in a vector of safe, temporary registers.
    std::vector<std::string> arg_result_regs;
    evaluateArguments(arg_result_regs);

    // 3. Determine if the target is a float function
    bool is_float_call = isFloatCallTarget(node);

    // 4. Just before the call, move the values from the temporary registers
    //    into the final argument registers (X0, X1, ...) or (D0, D1, ...) for floats.
    moveArgumentsToCallRegisters(arg_result_regs, is_float_call);

    // 5. Perform the routine call (BL or BLR).
    if (node.routine_expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
        VariableAccess* var_access = static_cast<VariableAccess*>(node.routine_expr.get());
        debug_print("Call target is a VariableAccess: " + var_access->name);

        if (RuntimeManager::instance().is_function_registered(var_access->name)) {
            debug_print("  Identified as a RUNTIME routine.");
            handleRuntimeCall(var_access);
        } else if (ASTAnalyzer::getInstance().get_function_metrics().count(var_access->name)) {
            debug_print("  Identified as a LOCAL (user-defined) routine.");
            emit(Encoder::create_branch_with_link(var_access->name));
            debug_print("  Generated BL call to routine '" + var_access->name + "'.");
        } else {
            debug_print("  Identified as potentially a LOCAL routine or function pointer via VariableAccess.");
            handleGeneralRoutineCall(node.routine_expr.get());
        }
    } else {
        debug_print("Call target is a complex expression (not a simple variable access).");
        handleGeneralRoutineCall(node.routine_expr.get());
    }

    // 6. Restore in-use caller-saved registers from the stack
    // Pass the exact list that was saved and the alignment flag.
    restoreCallerSavedRegisters(align_stack_needed_after_save, actual_saved_caller_saved_regs);

    // 7. Handle the return value
    debug_print("Handling return value...");
    if (is_float_call) {
        expression_result_reg_ = "D0"; // Float return value is in D0
        register_manager_.mark_register_as_used("D0");
        debug_print("  Return value expected in D0 (Float call). D0 marked as used.");
    } else {
        expression_result_reg_ = "X0"; // Integer return value is in X0
        register_manager_.mark_register_as_used("X0");
        debug_print("  Return value expected in X0 (Integer call). X0 marked as used.");
    }

    debug_print("--- Exiting NewCodeGenerator::visit(RoutineCallStatement& node) ---");
}
