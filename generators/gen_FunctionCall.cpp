#include "../NewCodeGenerator.h"
#include "../LabelManager.h"
#include "../analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>
#include <cstdint>
#include "CodeGenUtils.h"

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
                // Function is too far away - use the register-based approach
                debug_print("Runtime function '" + func_name + "' requires indirect branch - using BLR");
                
                // Check if we already have this function's address in a register
                std::string func_addr_reg = register_manager_.get_cached_routine_reg(func_name);

                if (func_addr_reg.empty()) {
                    // Cache MISS: Get a register from the cache pool and load the address.
                    func_addr_reg = register_manager_.get_reg_for_cache_eviction(func_name);

                    // This tells the linker to patch the absolute address of 'func_name'
                    // Generate the MOVZ/MOVK sequence using the JIT-specific function.
                    auto mov_instructions = Encoder::create_movz_movk_jit_addr(
                        func_addr_reg,
                        reinterpret_cast<uint64_t>(function_address),
                        func_name
                    );

                    // Tag all MOVZ/MOVK instructions with JitAddress.
                    for (auto& instr : mov_instructions) {
                        instr.jit_attribute = JITAttribute::JitAddress;
                    }

                    // Emit the sequence.
                    emit(mov_instructions);
                    
                } else {
                    // Cache HIT.
                    
                }

                // Branch to the register holding the runtime function's address.
                Instruction blr_instr = Encoder::create_branch_with_link_register(func_addr_reg);
                blr_instr.jit_attribute = JITAttribute::JitCall;
                blr_instr.target_label = func_name; // Set the function's name as the target label
                emit(blr_instr);
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
        emit(Encoder::create_branch_with_link_register(expression_result_reg_));
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
