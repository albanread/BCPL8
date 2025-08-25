#include "../NewCodeGenerator.h"
#include "../LabelManager.h"
#include "../analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include "CodeGenUtils.h"
#include "runtime/ListDataTypes.h"

// In generators/gen_FunctionCall.cpp

// Local utility functions for pushing and popping registers
namespace {
    // Push a register onto the stack (decrement SP, store register)
    void emit_push_reg(NewCodeGenerator* gen, const std::string& reg) {
        // SUB SP, SP, #16
        gen->emit(Encoder::create_sub_imm("SP", "SP", 16));
        // STR reg, [SP, #0]
        gen->emit(Encoder::create_str_imm(reg, "SP", 0));
    }

    // Pop a register from the stack (load register, increment SP)
    void emit_pop_reg(NewCodeGenerator* gen, const std::string& reg) {
        // LDR reg, [SP, #0]
        gen->emit(Encoder::create_ldr_imm(reg, "SP", 0));
        // ADD SP, SP, #16
        gen->emit(Encoder::create_add_imm("SP", "SP", 16));
        // Register marking is done in the main method
    }
}

void NewCodeGenerator::visit(FunctionCall& node) {
    debug_print("Visiting FunctionCall node for function call.");
    
    // Get function name if available through a VariableAccess
    std::string function_name;
    std::string target_func_name;
    if (auto* var_access = dynamic_cast<VariableAccess*>(node.function_expr.get())) {
        function_name = var_access->name;
        target_func_name = function_name;
    }

    // Check if this is a runtime function in our symbol table
    Symbol symbol;
    bool found_symbol = false;
    if (!function_name.empty()) {
        found_symbol = lookup_symbol(function_name, symbol);
    }

    // --- Special case for AS_INT, AS_FLOAT, AS_STRING, AS_LIST ---
    if (function_name == "AS_INT" || function_name == "AS_FLOAT" || function_name == "AS_STRING" || function_name == "AS_LIST") {
        if (node.arguments.size() != 1) {
            throw std::runtime_error(function_name + " expects exactly one argument.");
        }

        generate_expression_code(*node.arguments[0]);
        std::string ptr_reg = expression_result_reg_;
        std::string tag_reg = register_manager_.acquire_scratch_reg(*this);
        std::string good_type_label = label_manager_.create_label();
        int64_t expected_tag = 0;

        if (function_name == "AS_INT") expected_tag = ATOM_INT;       // 1
        if (function_name == "AS_FLOAT") expected_tag = ATOM_FLOAT;   // 2
        if (function_name == "AS_STRING") expected_tag = ATOM_STRING; // 3
        if (function_name == "AS_LIST") expected_tag = ATOM_LIST_POINTER;     // 4

        // --- Definitive Solution: Always type-check and extract value from ListAtom pointer ---
        // Step A: Load the type tag from the pointer.
        emit(Encoder::create_ldr_imm(tag_reg, ptr_reg, 0, "Load runtime type tag"));

        // Step B: Compare it to the expected tag and branch if correct.
        emit(Encoder::create_cmp_imm(tag_reg, expected_tag));
        register_manager_.release_register(tag_reg);
        emit(Encoder::create_branch_conditional("EQ", good_type_label));

        // Step C: If the type is wrong, halt.
        emit(Encoder::create_brk(1)); // Type mismatch error

        // Step D: If the type was correct, proceed to extract the value.
        instruction_stream_.define_label(good_type_label);

        if (function_name == "AS_LIST" || function_name == "AS_STRING") {
            // For pointers, load the pointer value from the atom's value field.
            std::string dest_x_reg = register_manager_.acquire_scratch_reg(*this);
            emit(Encoder::create_ldr_imm(dest_x_reg, ptr_reg, 8));
            if (function_name == "AS_STRING") {
                // Adjust the pointer to skip the 8-byte length prefix for strings.
                emit(Encoder::create_add_imm(dest_x_reg, dest_x_reg, 8));
            }
            expression_result_reg_ = dest_x_reg;
        } else if (function_name == "AS_FLOAT") {
            // For floats, load into a floating-point register.
            std::string dest_d_reg = register_manager_.acquire_fp_scratch_reg();
            emit(Encoder::create_ldr_fp_imm(dest_d_reg, ptr_reg, 8));
            expression_result_reg_ = dest_d_reg;
        } else { // AS_INT
            // For integers, load into a general-purpose register.
            std::string dest_x_reg = register_manager_.acquire_scratch_reg(*this);
            emit(Encoder::create_ldr_imm(dest_x_reg, ptr_reg, 8));
            expression_result_reg_ = dest_x_reg;
        }

        register_manager_.release_register(ptr_reg); // Release the original pointer register
        return;
    }

    // --- Special case for FIND(list, value) ---
    if (function_name == "FIND" && node.arguments.size() == 2) {
        // 1. Evaluate the list argument (goes into X0)
        generate_expression_code(*node.arguments[0]);
        emit(Encoder::create_mov_reg("X0", expression_result_reg_));
        register_manager_.release_register(expression_result_reg_);

        // 2. Evaluate the value argument
        generate_expression_code(*node.arguments[1]);
        std::string value_reg = expression_result_reg_;

        // 3. Determine the type and set the type tag in X2
        VarType value_type = ASTAnalyzer::getInstance().infer_expression_type(node.arguments[1].get());
        int64_t type_tag = (value_type == VarType::FLOAT) ? ATOM_FLOAT : ATOM_INT;

        // If the value is a float, its bits are in a D register. Move them to a GPR.
        if (register_manager_.is_fp_register(value_reg)) {
            emit(Encoder::create_fmov_reg("X1", value_reg)); // FMOV X1, D_val
        } else {
            emit(Encoder::create_mov_reg("X1", value_reg));
        }
        register_manager_.release_register(value_reg);

        // Load the type tag into X2
        emit(Encoder::create_movz_movk_abs64("X2", type_tag, ""));

        // Now call the runtime function
        emit(Encoder::create_branch_with_link("FIND"));

        // The result is in X0
        expression_result_reg_ = "X0";
        return;
    }

    // --- Special case for MAP(list, function) ---
    if (function_name == "MAP" && node.arguments.size() == 2) {
        // 1. Evaluate the list argument (goes into X0)
        generate_expression_code(*node.arguments[0]);
        emit(Encoder::create_mov_reg("X0", expression_result_reg_));
        register_manager_.release_register(expression_result_reg_);

        // 2. Load the ADDRESS of the mapping function into X1
        if (auto* map_var = dynamic_cast<VariableAccess*>(node.arguments[1].get())) {
            std::string map_name = map_var->name;
            emit(Encoder::create_adrp("X1", map_name));
            emit(Encoder::create_add_literal("X1", "X1", map_name));
        } else {
            throw std::runtime_error("Mapping function for MAP must be a function name.");
        }

        // Now call the runtime function
        emit(Encoder::create_branch_with_link("MAP"));

        // The result is in X0
        expression_result_reg_ = "X0";
        return;
    }

    // --- Special case for FILTER(list, predicate) ---
    if (function_name == "FILTER" && node.arguments.size() == 2) {
        // 1. Evaluate the list argument (goes into X0)
        generate_expression_code(*node.arguments[0]);
        emit(Encoder::create_mov_reg("X0", expression_result_reg_));
        register_manager_.release_register(expression_result_reg_);

        // 2. Load the ADDRESS of the predicate function into X1
        if (auto* predicate_var = dynamic_cast<VariableAccess*>(node.arguments[1].get())) {
            std::string predicate_name = predicate_var->name;
            emit(Encoder::create_adrp("X1", predicate_name));
            emit(Encoder::create_add_literal("X1", "X1", predicate_name));
        } else {
            throw std::runtime_error("Predicate for FILTER must be a function name.");
        }

        // Now call the runtime function
        emit(Encoder::create_branch_with_link("FILTER"));

        // The result is in X0
        expression_result_reg_ = "X0";
        return;
    }

    // --- STEP 1: Evaluate All Arguments and Store Results in Temporary Registers ---
    std::vector<std::string> arg_result_regs;
    for (const auto& arg_expr : node.arguments) {
        generate_expression_code(*arg_expr);
        std::string temp_reg;
        if (register_manager_.is_fp_register(expression_result_reg_)) {
            temp_reg = register_manager_.acquire_spillable_fp_temp_reg(*this);
            emit(Encoder::create_fmov_reg(temp_reg, expression_result_reg_));
        } else {
            temp_reg = register_manager_.acquire_spillable_temp_reg(*this);
            emit(Encoder::create_mov_reg(temp_reg, expression_result_reg_));
        }
        arg_result_regs.push_back(temp_reg);
    }

    // 1. Determine if the target is a float function based on its return type
    bool is_float_call = false;
    
    // Check symbol table first for type information
    if (found_symbol) {
        if (symbol.is_float_function()) {
            is_float_call = true;
            debug_print("Function '" + function_name + "' identified as float function from symbol table");
        } else if (symbol.is_runtime_function() || symbol.is_runtime_routine()) {
            debug_print("Function '" + function_name + "' identified as runtime " + 
                       (symbol.is_float_function() ? "float function" : "integer function") + 
                       " from symbol table");
            is_float_call = symbol.is_float_function();
        } else {
            debug_print("Function '" + function_name + "' identified as integer function from symbol table");
        }
    } else {
        // Fall back to legacy method if symbol not in symbol table
        // Use the target_func_name previously initialized
        
        if (auto* var_access = dynamic_cast<VariableAccess*>(node.function_expr.get())) {
            target_func_name = var_access->name;
            
            // Check for function return type in ASTAnalyzer (authoritative source)
            auto& return_types = ASTAnalyzer::getInstance().get_function_return_types();
            if (return_types.find(target_func_name) != return_types.end()) {
                debug_print("Function '" + target_func_name + "' found in AST analyzer return types map");
                is_float_call = (return_types.at(target_func_name) == VarType::FLOAT);
                if (is_float_call) {
                    debug_print("Function '" + target_func_name + "' identified as float function");
                } else {
                    debug_print("Function '" + target_func_name + "' identified as integer function");
                }
            } else if (RuntimeManager::instance().is_function_registered(target_func_name)) {
                // Check runtime functions
                is_float_call = (RuntimeManager::instance().get_function(target_func_name).type == FunctionType::FLOAT);
                debug_print("Runtime function '" + target_func_name + "' is " + 
                        (is_float_call ? "float" : "integer") + " type");
            } else {
                debug_print("Function '" + target_func_name + "' not found in return types - assuming integer type");
            }
        }
    }

    // 2. Just before the call, move the values from the temporary registers
    //    into the final argument registers (X0, X1, ...) or (D0, D1, ...) for floats.
    for (size_t i = 0; i < arg_result_regs.size(); ++i) {
        std::string src_reg = arg_result_regs[i];

        // Special-case for FPND: list pointer in X0, float value in D1
        if (function_name == "FPND") {
            if (i == 0) { // First argument (the list) goes to X0
                emit(Encoder::create_mov_reg("X0", src_reg));
            } else if (i == 1) { // Second argument (the float) goes to D1
                if (register_manager_.is_fp_register(src_reg)) {
                    emit(Encoder::create_fmov_reg("D1", src_reg));
                } else {
                    emit(Encoder::create_scvtf_reg("D1", src_reg));
                }
            }
            register_manager_.release_register(src_reg);
            continue;
        }

        // The register type needed depends on the function type (float vs integer)
        if (is_float_call) {
            // Float function expects arguments in D registers
            std::string dest_d_reg = "D" + std::to_string(i);
            debug_print("Setting up argument " + std::to_string(i) + " for float function call '" + target_func_name + "'");

            // Check if the source register is already a float register
            if (register_manager_.is_fp_register(src_reg)) {
                // NO CONVERSION NEEDED: Float (D) to Float (D)
                // Just move between D registers if needed
                if (src_reg != dest_d_reg) {
                    debug_print("Emitting FMOV: " + dest_d_reg + " <- " + src_reg + " (float to float)");
                    emit(Encoder::create_fmov_reg(dest_d_reg, src_reg));
                } else {
                    debug_print("Source register " + src_reg + " is already in the correct D register");
                }
            } else {
                // CONVERSION NEEDED: Integer (X) to Float (D)
                debug_print("Converting integer in " + src_reg + " to float in " + dest_d_reg + " (int to float)");
                emit(Encoder::create_scvtf_reg(dest_d_reg, src_reg));
            }
            register_manager_.mark_register_as_used(dest_d_reg);

        } else { 
            // Integer function expects arguments in X registers
            std::string dest_x_reg = "X" + std::to_string(i);
            debug_print("Setting up argument " + std::to_string(i) + " for integer function call '" + target_func_name + "'");
            if (register_manager_.is_fp_register(src_reg)) {
                // CONVERSION NEEDED: Float (D) to Integer (X)
                debug_print("Converting float in " + src_reg + " to integer in " + dest_x_reg + " (float to int)");
                generate_float_to_int_truncation(dest_x_reg, src_reg);
            } else {
                // NO CONVERSION NEEDED: Integer (X) to Integer (X)
                // Just move between X registers if needed
                if (src_reg != dest_x_reg) {
                    debug_print("Moving from " + src_reg + " to " + dest_x_reg + " (int to int)");
                    emit(Encoder::create_mov_reg(dest_x_reg, src_reg));
                } else {
                    debug_print("Source register " + src_reg + " is already in the correct X register");
                }
            }
            register_manager_.mark_register_as_used(dest_x_reg);
        }

        // Finally, release the temporary register that held the argument's value.
        register_manager_.release_register(src_reg);
    }

    // Perform the function call (BL or BLR).
    if (auto* var_access = dynamic_cast<VariableAccess*>(node.function_expr.get())) {
        const std::string& func_name = var_access->name;

        if (ASTAnalyzer::getInstance().get_function_metrics().count(func_name) > 0) {
            // It's a known user-defined function. Emit a direct relative call.
            emit(Encoder::create_branch_with_link(func_name));

        } else if (RuntimeManager::instance().is_function_registered(func_name)) {
            // It's a runtime function. Check if it's within range for a direct branch.
            void* function_address = RuntimeManager::instance().get_function(func_name).address;
            
            // Get the current instruction stream address
            size_t current_address = instruction_stream_.get_current_address();
            bool direct_branch_possible = false;
            
            // Only attempt direct branches if we have valid addresses to work with
            if (function_address != nullptr && current_address != 0) {
                direct_branch_possible = codegen_utils::is_within_branch_range(
                    current_address, 
                    reinterpret_cast<uint64_t>(function_address)
                );
            }
            
            if (direct_branch_possible) {
                // The function is within range for a direct branch - use BL instead of BLR
                debug_print("Runtime function '" + func_name + "' is within direct branch range - using BL");
                Instruction bl_instr = Encoder::create_branch_with_link(func_name);
                bl_instr.jit_attribute = JITAttribute::JitCall;
                emit(bl_instr);
            } else {
                // Use the X28-relative runtime function pointer table approach
                debug_print("Runtime function '" + func_name + "' using X28-relative pointer table (LDR+BLR).");

                // 1. Get the routine's pre-assigned table offset from the RuntimeManager.
                size_t offset = RuntimeManager::instance().get_function_offset(func_name);

                // 2. Acquire a temporary scratch register to hold the function address.
                std::string addr_reg = register_manager_.acquire_scratch_reg(*this);

                // 3. Emit the LDR instruction to load the pointer from the global table.
                Instruction ldr_instr = Encoder::create_ldr_imm(addr_reg, "X19", offset);
                ldr_instr.jit_attribute = JITAttribute::JitAddress;
                emit(ldr_instr);

                // 4. Emit the indirect branch.
                Instruction blr_instr = Encoder::create_branch_with_link_register(addr_reg);
                blr_instr.jit_attribute = JITAttribute::JitCall;
                blr_instr.target_label = func_name;
                blr_instr.assembly_text += " ; call " + func_name;
                emit(blr_instr);

                // 5. Release the scratch register.
                register_manager_.release_register(addr_reg);
            }
        } else {
            // It's not a known function, so treat it as a variable holding a function pointer.
            generate_expression_code(*var_access); // Get the pointer's value
            emit(Encoder::create_branch_with_link_register(expression_result_reg_));
            register_manager_.release_register(expression_result_reg_);
        }
    } else {
        // The function is a complex expression; evaluate it to get a function pointer.
        generate_expression_code(*node.function_expr);
        Instruction blr_instr = Encoder::create_branch_with_link_register(expression_result_reg_);
        blr_instr.jit_attribute = JITAttribute::JitCall;
        emit(blr_instr);
        register_manager_.release_register(expression_result_reg_);
    }

    // Handle the return value based on the function's return type
    if (is_float_call) {
        expression_result_reg_ = "D0"; // Float return value is in D0
        register_manager_.mark_register_as_used("D0");
        debug_print("Function call to '" + target_func_name + "' returned float value in D0");
    } else {
        expression_result_reg_ = "X0"; // Integer return value is in X0
        register_manager_.mark_register_as_used("X0");
        debug_print("Function call to '" + target_func_name + "' returned integer value in X0");
    }
}
