#include "DataTypes.h"
#include "NewCodeGenerator.h"
#include <unordered_set>
#include "Encoder.h"
#include "RegisterManager.h"
#include "LabelManager.h"
#include "DataGenerator.h"
#include "CallFrameManager.h"
#include "analysis/ASTAnalyzer.h" // Required for analyzer_ and its methods
#include "AST.h" // Required for ASTNode types, Expression, Statement, etc.
#include "ASTVisitor.h"
#include <iostream>   // For std::cout, std::cerr
#include <stdexcept>  // For std::runtime_error
#include <algorithm>  // For std::sort
#include <sstream>    // For std::stringstream
#include <vector>     // For std::vector
#include <map>        // For std::map
#include <stack>      // For std::stack
#include "analysis/LiveInterval.h"




// --- Constructor ---
// Initializes all member references and unique_ptr.
NewCodeGenerator::NewCodeGenerator(InstructionStream& instruction_stream,
                                   RegisterManager& register_manager,
                                   LabelManager& label_manager,
                                   bool debug,
                                   int debug_level,
                                   DataGenerator& data_generator,
                                   uint64_t data_base_addr,
                                   const CFGBuilderPass& cfg_builder,
                                   std::unique_ptr<SymbolTable> symbol_table)
    : instruction_stream_(instruction_stream),
      register_manager_(register_manager),
      label_manager_(label_manager),
      debug_enabled_(debug),
      debug_level(debug_level),
      data_generator_(data_generator),
      data_segment_base_addr_(data_base_addr),
      cfg_builder_(cfg_builder),
      symbol_table_(std::move(symbol_table)) {
    x28_is_loaded_in_current_function_ = false;
}

// Look up a symbol in the symbol table
void NewCodeGenerator::visit(BinaryOp& node) {
    debug_print("Visiting BinaryOp node.");

    // For LogicalAnd and LogicalOr operations, implement short-circuit logic
    if (node.op == BinaryOp::Operator::LogicalAnd) {
        debug_print("Generating short-circuit code for LogicalAnd");
        generate_short_circuit_and(node);
        return;
    } else if (node.op == BinaryOp::Operator::LogicalOr) {
        debug_print("Generating short-circuit code for LogicalOr");
        generate_short_circuit_or(node);
        return;
    }

    // For all other binary operations, use the standard approach
    generate_expression_code(*node.left);
    std::string left_reg = expression_result_reg_;

    generate_expression_code(*node.right);
    std::string right_reg = expression_result_reg_;

    // Handle different binary operators
    switch (node.op) {
        case BinaryOp::Operator::Add:
            emit(Encoder::create_add_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::Subtract:
            emit(Encoder::create_sub_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::Multiply:
            emit(Encoder::create_mul_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::Divide:
            emit(Encoder::create_sdiv_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::Remainder:
            // First perform division
            emit(Encoder::create_sdiv_reg("X16", left_reg, right_reg)); // Temporary register X16
            // Then multiply quotient * divisor
            emit(Encoder::create_mul_reg("X16", "X16", right_reg));
            // Finally subtract to get remainder
            emit(Encoder::create_sub_reg(left_reg, left_reg, "X16"));
            break;
        case BinaryOp::Operator::Equal:
            emit(Encoder::create_cmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset_eq(left_reg));
            break;
        case BinaryOp::Operator::NotEqual:
            emit(Encoder::create_cmp_reg(left_reg, right_reg));
            emit(Encoder::create_csetm_ne(left_reg));
            break;
        case BinaryOp::Operator::Less:
            emit(Encoder::create_cmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "LT"));
            break;
        case BinaryOp::Operator::LessEqual:
            emit(Encoder::create_cmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "LE"));
            break;
        case BinaryOp::Operator::Greater:
            emit(Encoder::create_cmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "GT"));
            break;
        case BinaryOp::Operator::GreaterEqual:
            emit(Encoder::create_cmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "GE"));
            break;
        case BinaryOp::Operator::LogicalAnd:
            emit(Encoder::create_and_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::LogicalOr:
            emit(Encoder::create_orr_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::Equivalence:
            // Use logical XOR for equivalence
            emit(Encoder::create_cmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "EQ"));
            break;
        case BinaryOp::Operator::LeftShift:
            emit(Encoder::create_lsl_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::RightShift:
            emit(Encoder::create_lsr_reg(left_reg, left_reg, right_reg));
            break;
        // Handle float operations
        case BinaryOp::Operator::FloatAdd:
            emit(Encoder::create_fadd_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::FloatSubtract:
            emit(Encoder::create_fsub_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::FloatMultiply:
            emit(Encoder::create_fmul_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::FloatDivide:
            emit(Encoder::create_fdiv_reg(left_reg, left_reg, right_reg));
            break;
        case BinaryOp::Operator::FloatEqual:
            emit(Encoder::create_fcmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "EQ"));
            break;
        case BinaryOp::Operator::FloatNotEqual:
            emit(Encoder::create_fcmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "NE"));
            break;
        case BinaryOp::Operator::FloatLess:
            emit(Encoder::create_fcmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "LT"));
            break;
        case BinaryOp::Operator::FloatLessEqual:
            emit(Encoder::create_fcmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "LE"));
            break;
        case BinaryOp::Operator::FloatGreater:
            emit(Encoder::create_fcmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "GT"));
            break;
        case BinaryOp::Operator::FloatGreaterEqual:
            emit(Encoder::create_fcmp_reg(left_reg, right_reg));
            emit(Encoder::create_cset(left_reg, "GE"));
            break;
        default:
            // Handle unknown or unsupported operators
            std::cerr << "Error: Unknown binary operator.\n";
            break;
    }

    // After the operation, the right-hand side register is no longer needed and can be freed.
    register_manager_.release_register(right_reg);
    debug_print("Released right-hand operand register: " + right_reg);

    // The result of all binary operations is stored in the left_reg
    expression_result_reg_ = left_reg;

    debug_print("Finished visiting BinaryOp node. Result in " + expression_result_reg_);
}

// Generates code for short-circuit logical AND (&&)
void NewCodeGenerator::generate_short_circuit_and(BinaryOp& node) {
    // Generate code for the left operand
    generate_expression_code(*node.left);
    std::string condition_reg = expression_result_reg_;

    // Create labels for the false case and the end
    std::string false_label = label_manager_.create_label();
    std::string end_label = label_manager_.create_label();

    // If the left operand is false (0), jump to the false label
    // In BCPL, 0 is false, anything else is true
    emit(Encoder::create_cmp_imm(condition_reg, 0));
    emit(Encoder::create_branch_conditional("EQ", false_label));

    // Left operand is true, evaluate the right operand
    generate_expression_code(*node.right);
    // The result is already in expression_result_reg_

    // Unconditionally jump to end
    emit(Encoder::create_branch_unconditional(end_label));

    // False case: just load 0 (false)
    emit(Encoder::create_directive("." + false_label + ":"));
    emit(Encoder::create_movz_imm(expression_result_reg_, 0, 0));

    // End label
    emit(Encoder::create_directive("." + end_label + ":"));
}

// Generates code for short-circuit logical OR (||)
void NewCodeGenerator::generate_short_circuit_or(BinaryOp& node) {
    // Generate code for the left operand
    generate_expression_code(*node.left);
    std::string condition_reg = expression_result_reg_;

    // Create labels for the true case and the end
    std::string true_label = label_manager_.create_label();
    std::string end_label = label_manager_.create_label();

    // If the left operand is true (not 0), jump to the true label
    emit(Encoder::create_cmp_imm(condition_reg, 0));
    emit(Encoder::create_branch_conditional("NE", true_label));

    // Left operand is false, evaluate the right operand
    generate_expression_code(*node.right);
    // The result is already in expression_result_reg_

    // Unconditionally jump to end
    emit(Encoder::create_branch_unconditional(end_label));

    // True case: load -1 (true in BCPL)
    emit(Encoder::create_directive("." + true_label + ":"));
    emit(Encoder::create_movz_imm(expression_result_reg_, 0xFFFF, 0));
    emit(Encoder::create_movk_imm(expression_result_reg_, 0xFFFF, 1));
    emit(Encoder::create_movk_imm(expression_result_reg_, 0xFFFF, 2));
    emit(Encoder::create_movk_imm(expression_result_reg_, 0xFFFF, 3));

    // End label
    emit(Encoder::create_directive("." + end_label + ":"));
}

bool NewCodeGenerator::lookup_symbol(const std::string& name, Symbol& symbol) const {
    if (!symbol_table_) {
        return false;
    }
    return symbol_table_->lookup(name, symbol);
}

// --- Linear Scan Register Allocation ---
void NewCodeGenerator::performLinearScan(const std::string& functionName, const std::vector<LiveInterval>& intervals) {
    debug_print("Performing linear scan register allocation for function: " + functionName);

    // Don't clear the existing allocations - we might need to keep non-local variables

    // Make sure we have a valid frame manager
    if (!current_frame_manager_) {
        debug_print("WARNING: No current frame manager when performing linear scan for " + functionName);
        return;
    }

    // First, check if we have any intervals to process
    if (intervals.empty()) {
        debug_print("No intervals to process for function: " + functionName);
        return;
    }

    // Check if the prologue has been generated, which finalized variable offsets
    bool is_prologue_ready = false;
    try {
        // This is just a test to see if we can get offsets
        if (!intervals.empty() && current_frame_manager_->has_local(intervals[0].var_name)) {
            current_frame_manager_->get_offset(intervals[0].var_name);
            is_prologue_ready = true;
        }
    } catch (const std::runtime_error& e) {
        debug_print("WARNING: Prologue not yet generated, cannot determine stack offsets: " + std::string(e.what()));
    }

    // 1. Sort intervals by their starting point
    std::vector<LiveInterval> sorted_intervals = intervals;
    std::sort(sorted_intervals.begin(), sorted_intervals.end(),
        [](const LiveInterval& a, const LiveInterval& b) {
            return a.start_point < b.start_point;
        });

    // 2. Initialize state
    std::vector<LiveInterval> active_int;  // Active intervals using int registers
    std::vector<LiveInterval> active_fp;   // Active intervals using float registers

    // Prioritize the use of callee-saved registers for variables
    std::vector<std::string> free_int_registers = RegisterManager::VARIABLE_REGS;
    std::vector<std::string> free_fp_registers = RegisterManager::FP_VARIABLE_REGS;

    // Ensure FP_VARIABLE_REGS (D8-D15) are prioritized for floating-point variables
    // By reversing the order, we'll use D8, D9, etc. first (which are callee-saved)
    std::reverse(free_fp_registers.begin(), free_fp_registers.end());

    debug_print("Available INT registers for allocation: " +
                std::to_string(free_int_registers.size()));
    debug_print("Available FP registers for allocation: " +
                std::to_string(free_fp_registers.size()));

    // Track assigned registers to avoid duplicates
    std::unordered_map<std::string, bool> register_in_use;
    for (const auto& reg : RegisterManager::VARIABLE_REGS) {
        register_in_use[reg] = false;
    }
    for (const auto& reg : RegisterManager::FP_VARIABLE_REGS) {
        register_in_use[reg] = false;
    }

    // Track maximum register pressure
    size_t max_int_pressure = 0;
    size_t max_fp_pressure = 0;

    // 3. Main allocation loop
    for (auto interval : sorted_intervals) {
        debug_print("Processing interval for: " + interval.var_name +
                    " (start=" + std::to_string(interval.start_point) +
                    ", end=" + std::to_string(interval.end_point) + ")");

        // Skip runtime functions or global variables that don't need register allocation
        if (interval.var_name == "WRITES" || interval.var_name == "WRITEN" ||
            interval.var_name == "WRITEF" || interval.var_name == "READN" ||
            data_generator_.is_global_variable(interval.var_name)) {
            debug_print("Skipping runtime function or global variable: " + interval.var_name);
            continue;
        }

        // Make sure the variable is registered with the frame manager
        if (!current_frame_manager_->has_local(interval.var_name)) {
            debug_print("WARNING: Variable " + interval.var_name + " not registered in frame manager. Adding it.");
            current_frame_manager_->add_local(interval.var_name);
        }

        // Determine if this variable is a float
        VarType var_type = ASTAnalyzer::getInstance().get_variable_type(functionName, interval.var_name);
        bool is_float = (var_type == VarType::FLOAT);

        // Expire old intervals from appropriate active lists based on their register type
        auto expire_intervals = [&interval, &register_in_use, this](
            LiveInterval& active_interval, std::vector<std::string>& free_regs) {
            if (active_interval.end_point < interval.start_point) {
                debug_print("Expiring interval for: " + active_interval.var_name);
                // Free the register
                if (!active_interval.is_spilled) {
                    free_regs.push_back(active_interval.assigned_register);
                    register_in_use[active_interval.assigned_register] = false;
                }
                return true;
            }
            return false;
        };

        // Expire integer intervals
        active_int.erase(
            std::remove_if(active_int.begin(), active_int.end(),
                [&expire_intervals, &free_int_registers](LiveInterval& active_interval) {
                    return expire_intervals(active_interval, free_int_registers);
                }
            ),
            active_int.end()
        );

        // Expire floating-point intervals
        active_fp.erase(
            std::remove_if(active_fp.begin(), active_fp.end(),
                [&expire_intervals, &free_fp_registers](LiveInterval& active_interval) {
                    return expire_intervals(active_interval, free_fp_registers);
                }
            ),
            active_fp.end()
        );

        // Track register pressure
        size_t int_pressure = active_int.size();
        size_t fp_pressure = active_fp.size();
        debug_print("Integer register pressure: " + std::to_string(int_pressure) +
                    ", Float register pressure: " + std::to_string(fp_pressure));

        // Update maximum pressure
        max_int_pressure = std::max(max_int_pressure, int_pressure);
        max_fp_pressure = std::max(max_fp_pressure, fp_pressure);

        // Choose the appropriate register pool based on variable type
        std::vector<std::string>& free_registers = is_float ? free_fp_registers : free_int_registers;
        std::vector<LiveInterval>& active_list = is_float ? active_fp : active_int;

        if (free_registers.empty()) {
            // No free registers, need to spill
            // Find the interval in active that has the longest remaining lifetime from current point
            auto spill_candidate = std::max_element(active_list.begin(), active_list.end(),
                [&interval](const LiveInterval& a, const LiveInterval& b) {
                    // Consider remaining lifetime from current point (not total lifetime)
                    size_t a_remaining = a.end_point - interval.start_point;
                    size_t b_remaining = b.end_point - interval.start_point;
                    return a_remaining < b_remaining;
                });

            if (spill_candidate != active_list.end() && spill_candidate->end_point > interval.end_point) {
                // Spill the new interval
                debug_print("Spilling new interval: " + interval.var_name +
                            " (float=" + std::to_string(is_float) + ")");
                interval.is_spilled = true;

                // Only try to get stack offset if prologue is ready
                if (is_prologue_ready) {
                    try {
                        interval.stack_offset = current_frame_manager_->get_offset(interval.var_name);
                    } catch (const std::runtime_error& e) {
                        // If there's an error, we'll set the offset later when the prologue is generated
                        debug_print("Cannot get stack offset for " + interval.var_name + ": " + e.what());
                        interval.stack_offset = -1;  // Mark as invalid
                    }
                } else {
                    // We'll set the actual offset after prologue generation
                    interval.stack_offset = -1;
                }
            } else {
                // Spill the candidate with the longest remaining lifetime
                debug_print("Spilling active interval: " + spill_candidate->var_name);
                std::string freed_reg = spill_candidate->assigned_register;
                spill_candidate->is_spilled = true;

                // Only try to get stack offset if prologue is ready
                if (is_prologue_ready) {
                    try {
                        spill_candidate->stack_offset = current_frame_manager_->get_offset(spill_candidate->var_name);
                    } catch (const std::runtime_error& e) {
                        debug_print("Cannot get stack offset for " + spill_candidate->var_name + ": " + e.what());
                        spill_candidate->stack_offset = -1;  // Mark as invalid
                    }
                } else {
                    spill_candidate->stack_offset = -1;
                }

                // Assign the freed register to our current interval
                interval.assigned_register = freed_reg;
                register_in_use[freed_reg] = true;
            }
        } else {
            // We have a free register, assign it
            std::string reg = free_registers.back();
            free_registers.pop_back();
            interval.assigned_register = reg;
            register_in_use[reg] = true;

            // If this is a float variable, make sure the register manager knows about it
            if (is_float && current_frame_manager_) {
                current_frame_manager_->mark_variable_as_float(interval.var_name);
            }

            debug_print("Assigned register " + reg + " to " + interval.var_name +
                        " (float=" + std::to_string(is_float) + ", reg_type=" +
                        (reg[0] == 'D' ? "float" : "int") + ")");
        }

        // Add the current interval to the appropriate active list
        if (is_float) {
            active_fp.push_back(interval);
        } else {
            active_int.push_back(interval);
        }

        // Update our allocation map
        current_function_allocation_[interval.var_name] = interval;
    }

    // Print allocation plan and register pressure stats for debugging
    debug_print("Linear scan allocation complete for function: " + functionName);
    debug_print("Maximum register pressure: INT=" + std::to_string(max_int_pressure) +
                " (of " + std::to_string(RegisterManager::VARIABLE_REGS.size()) + "), " +
                "FLOAT=" + std::to_string(max_fp_pressure) +
                " (of " + std::to_string(RegisterManager::FP_VARIABLE_REGS.size()) + ")");

    // Store the maximum pressure values for later reporting
    max_int_pressure_ = std::max(max_int_pressure_, max_int_pressure);
    max_fp_pressure_ = std::max(max_fp_pressure_, max_fp_pressure);

    // Verify type consistency and print allocation details
    for (const auto& [var_name, alloc] : current_function_allocation_) {
        if (alloc.is_spilled) {
            if (alloc.stack_offset != -1) {
                debug_print("  " + var_name + " => SPILLED (stack offset: " + std::to_string(alloc.stack_offset) + ")");
            } else {
                debug_print("  " + var_name + " => SPILLED (stack offset pending prologue generation)");
            }
        } else {
            // Verify type consistency
            VarType var_type = ASTAnalyzer::getInstance().get_variable_type(functionName, var_name);
            bool is_float = (var_type == VarType::FLOAT);
            bool got_float_reg = (alloc.assigned_register[0] == 'D' || alloc.assigned_register[0] == 'S');

            debug_print("  " + var_name + " => " + alloc.assigned_register +
                       " (type=" + (is_float ? "float" : "int") + ")");

            if (is_float != got_float_reg) {
                debug_print("  WARNING: Type-register mismatch for " + var_name +
                          ": expected " + (is_float ? "float" : "int") +
                          " register, got " + alloc.assigned_register);
            }
        }
    }
}

// Updates the stack offsets for all spilled variables after prologue generation
void NewCodeGenerator::update_spill_offsets() {
if (!current_frame_manager_) {
    debug_print("WARNING: No current frame manager when updating spill offsets");
    return;
}

bool any_updates = false;
for (auto& [var_name, interval] : current_function_allocation_) {
    if (var_name == "WRITES" || var_name == "WRITEN" ||
        var_name == "WRITEF" || var_name == "READN" ||
        data_generator_.is_global_variable(var_name)) {
        // Skip runtime functions and globals
        continue;
    }

    if (interval.is_spilled && interval.stack_offset == -1) {
        try {
            if (current_frame_manager_->has_local(var_name)) {
                interval.stack_offset = current_frame_manager_->get_offset(var_name);
                debug_print("Updated stack offset for spilled variable '" + var_name +
                            "': " + std::to_string(interval.stack_offset));
                any_updates = true;
            } else {
                // Add it to the frame manager if it's not there
                debug_print("Adding missing variable '" + var_name + "' to frame manager");
                current_frame_manager_->add_local(var_name);

                // Now try to get its offset
                try {
                    interval.stack_offset = current_frame_manager_->get_offset(var_name);
                    debug_print("Added and updated stack offset for variable '" + var_name +
                                "': " + std::to_string(interval.stack_offset));
                    any_updates = true;
                } catch (const std::runtime_error& e) {
                    debug_print("ERROR: Failed to get offset after adding variable '" + var_name +
                                "': " + std::string(e.what()));
                }
            }
        } catch (const std::runtime_error& e) {
            debug_print("ERROR: Failed to get offset for spilled variable '" + var_name +
                        "': " + std::string(e.what()));
        }
    }
}

    if (any_updates) {
        debug_print("Updated stack offsets for spilled variables after prologue generation");
    }
}



// --- Public Entry Point Methods ---

// The main entry point for starting code generation.
void NewCodeGenerator::generate_code(Program& program) {
    debug_print("Starting code generation for program.");
    // Delegates to visit(Program& node), which is implemented in generators/gen_Program.cpp.
    // This starts the AST traversal.
    visit(program);
    data_generator_.calculate_global_offsets();
    debug_print("Code generation finished.");
}

// Generic fallback for unsupported expressions
void NewCodeGenerator::visit(Expression& node) {
    debug_print("Visiting generic Expression node.");
    // Handle unsupported expressions or log a warning
    std::cerr << "[WARNING] Unsupported expression encountered during code generation." << std::endl;
}

// --- Assignment helpers ---

void NewCodeGenerator::handle_variable_assignment(VariableAccess* var_access, const std::string& value_to_store_reg) {
    debug_print("Handling assignment for variable: " + var_access->name);

    bool is_source_float = register_manager_.is_fp_register(value_to_store_reg);
    VarType dest_type = ASTAnalyzer::getInstance().get_variable_type(current_function_name_, var_access->name);
    bool is_dest_float = (dest_type == VarType::FLOAT);

    std::string final_reg_to_store = value_to_store_reg;
    bool reg_was_converted = false;

    if (is_source_float && !is_dest_float) {
        // Coerce float value to integer
        debug_print("Coercing float value from " + value_to_store_reg + " to integer for variable '" + var_access->name + "'.");
        std::string int_reg = register_manager_.acquire_scratch_reg();
        emit(Encoder::create_fcvtzs_reg(int_reg, value_to_store_reg)); // Convert float to signed int
        final_reg_to_store = int_reg;
        reg_was_converted = true;
    } else if (!is_source_float && is_dest_float) {
        // Coerce integer value to float
        debug_print("Coercing integer value from " + value_to_store_reg + " to float for variable '" + var_access->name + "'.");
        std::string fp_reg = register_manager_.acquire_fp_scratch_reg();
        emit(Encoder::create_scvtf_reg(fp_reg, value_to_store_reg)); // Convert signed int to float
        final_reg_to_store = fp_reg;
        reg_was_converted = true;
    }

    store_variable_register(var_access->name, final_reg_to_store);

    // Release the original register if a new one was created for conversion.
    if (reg_was_converted) {
        register_manager_.release_register(value_to_store_reg);
    }
    // Always release the final register used for the store.
    register_manager_.release_register(final_reg_to_store);
}

void NewCodeGenerator::handle_vector_assignment(VectorAccess* vec_access, const std::string& value_to_store_reg) {
    // 1. Evaluate vector_expr to get the base address (e.g., V)
    generate_expression_code(*vec_access->vector_expr);
    std::string vector_base_reg = expression_result_reg_;

    // 2. Evaluate index_expr to get the index (e.g., 0)
    generate_expression_code(*vec_access->index_expr);
    std::string index_reg = expression_result_reg_;

    // 3. Calculate the byte offset: index * 8 (for 64-bit words)
    emit(Encoder::create_lsl_imm(index_reg, index_reg, 3));
    debug_print("Calculated byte offset for vector assignment.");

    // 4. Calculate the effective memory address: base + offset
    std::string effective_addr_reg = register_manager_.get_free_register();
    emit(Encoder::create_add_reg(effective_addr_reg, vector_base_reg, index_reg));
    debug_print("Calculated effective address for vector assignment.");

    // Release registers used for address calculation
    register_manager_.release_register(vector_base_reg);
    register_manager_.release_register(index_reg);

    // 5. Store the RHS value to the effective address
    emit(Encoder::create_str_imm(value_to_store_reg, effective_addr_reg, 0));
    debug_print("Stored value to vector element.");

    // Release registers used in the store
    register_manager_.release_register(value_to_store_reg);
    register_manager_.release_register(effective_addr_reg);
}

void NewCodeGenerator::handle_char_indirection_assignment(CharIndirection* char_indirection, const std::string& value_to_store_reg) {
    // 1. Evaluate string_expr to get the base address (e.g., S)
    generate_expression_code(*char_indirection->string_expr);
    std::string string_base_reg = expression_result_reg_;

    // 2. Evaluate index_expr to get the index (e.g., 0)
    generate_expression_code(*char_indirection->index_expr);
    std::string index_reg = expression_result_reg_;

    // 3. Calculate the byte offset: index * 4 (for 32-bit characters in BCPL)
    emit(Encoder::create_lsl_imm(index_reg, index_reg, 2));
    debug_print("Calculated byte offset for char indirection assignment.");

    // 4. Calculate the effective memory address: base + offset
    std::string effective_addr_reg = register_manager_.get_free_register();
    emit(Encoder::create_add_reg(effective_addr_reg, string_base_reg, index_reg));
    debug_print("Calculated effective address for char indirection assignment.");

    // Release registers used for address calculation
    register_manager_.release_register(string_base_reg);
    register_manager_.release_register(index_reg);

    // 5. Store the RHS value to the effective address (using STR for 32-bit word for char)
    // FIX: Use the new 32-bit store instruction for character assignment
    emit(Encoder::create_str_word_imm(value_to_store_reg, effective_addr_reg, 0));
    debug_print("Stored value to character element.");

    // Release registers used in the store
    register_manager_.release_register(value_to_store_reg);
    register_manager_.release_register(effective_addr_reg);
}



// --- Common Helper Methods (private implementations) ---
// These methods are shared across various visit functions and encapsulate common tasks.

// Emits a sequence of instructions to the instruction stream.
void NewCodeGenerator::emit(const std::vector<Instruction>& instrs) {
    for (const auto& instr : instrs) {
        instruction_stream_.add(instr);
    }
}

// Emits a single instruction to the instruction stream.
void NewCodeGenerator::emit(const Instruction& instr) {
    debug_print_level("Emitting instruction: " + instr.assembly_text, 5);
    instruction_stream_.add(instr);
}

// Helper for general debugging output.
void NewCodeGenerator::debug_print(const std::string& message) const {
    if (debug_enabled_) {
        std::cout << "[DEBUG] " << message << std::endl;
    }
}

// Helper for debugging output with a configurable level.
void NewCodeGenerator::debug_print_level(const std::string& message, int level) const {
    if (debug_enabled_ && level <= debug_level) {
        std::cout << "[DEBUG LEVEL " << level << "] " << message << std::endl;
    }
}

// Enters a new scope by saving the current symbol table and clearing it.
// Used for BLOCK statements, function/routine bodies, etc.
void NewCodeGenerator::enter_scope() {
    scope_stack_.push(current_scope_symbols_); // Save the current scope
    current_scope_symbols_.clear();            // Start a fresh scope
    debug_print("Entered new scope. Scope stack size: " + std::to_string(scope_stack_.size()));
}

// Exits the current scope by restoring the previous symbol table.
void NewCodeGenerator::exit_scope() {
    if (scope_stack_.empty()) {
        throw std::runtime_error("Attempted to exit scope when no scope was active.");
    }
    current_scope_symbols_ = scope_stack_.top(); // Restore previous scope
    scope_stack_.pop();                          // Remove from stack
    debug_print("Exited scope. Scope stack size: " + std::to_string(scope_stack_.size()));
}

// Handles the common logic for recursively generating code for an expression.
// The result of the expression should be stored in 'expression_result_reg_'.
void NewCodeGenerator::generate_expression_code(Expression& expr) {
    // Delegates to the specific visit method for the expression type (e.g., visit(NumberLiteral&)).
    // That visit method is responsible for setting expression_result_reg_.
    expr.accept(*this);
}

// Handles the common logic for recursively generating code for a statement.
// Statements do not typically have a "result" register.
void NewCodeGenerator::generate_statement_code(Statement& stmt) {
    // Delegates to the specific visit method for the statement type (e.g., visit(AssignmentStatement&)).
    stmt.accept(*this);
}

// Processes a list of declarations (e.g., from Program or BlockStatement).
void NewCodeGenerator::process_declarations(const std::vector<DeclPtr>& declarations) {
    for (const auto& decl_ptr : declarations) {
        if (decl_ptr) {
            debug_print_level(
                std::string("process_declarations: About to process declaration node at ") +
                std::to_string(reinterpret_cast<uintptr_t>(decl_ptr.get())),
                4
            );
            process_declaration(*decl_ptr); // Calls the helper for a single declaration
        }
    }
}

// Processes a single declaration.
void NewCodeGenerator::process_declaration(Declaration& decl) {
    // This helper ensures that the correct polymorphic visit method is called
    // based on the declaration's dynamic type (e.g., visit(LetDeclaration&)).
    decl.accept(*this);
}

void NewCodeGenerator::generate_float_to_int_truncation(const std::string& dest_x_reg, const std::string& src_d_reg) {
    // FCVTZS <Xd>, <Dn> (Floating-point Convert to Signed integer, rounding toward Zero)
    emit(Encoder::create_fcvtzs_reg(dest_x_reg, src_d_reg));
    debug_print("Generated FCVTZS to truncate " + src_d_reg + " to " + dest_x_reg);
}


// --- Variable Access/Storage Helpers ---
// These methods manage loading values from variables into registers and storing values from registers back to variables.
std::string NewCodeGenerator::get_variable_register(const std::string& var_name) {
    debug_print("get_variable_register called for: '" + var_name + "'");

    // Look up the symbol in our symbol table if available
    Symbol symbol;
    bool found_symbol = lookup_symbol(var_name, symbol);

    // Check for runtime functions that shouldn't be allocated
    if (found_symbol && symbol.is_runtime()) {
        std::string reg = register_manager_.acquire_scratch_reg();
        debug_print("Using scratch register for runtime function: " + var_name);
        return reg;
    }

    // First, check if the variable is a local allocated by the linear scan allocator
    auto it = current_function_allocation_.find(var_name);
    if (it != current_function_allocation_.end()) {
        const LiveInterval& allocation = it->second;

        if (!allocation.is_spilled) {
            // Determine if we're using the correct register type
            bool is_float = false;
            if (found_symbol) {
                is_float = (symbol.type == VarType::FLOAT);
            } else {
                // Fall back to ASTAnalyzer if symbol not found
                VarType var_type = ASTAnalyzer::getInstance().get_variable_type(current_function_name_, var_name);
                is_float = (var_type == VarType::FLOAT);
            }

            bool got_float_reg = (allocation.assigned_register[0] == 'D' || allocation.assigned_register[0] == 'S');

            if (is_float != got_float_reg) {
                debug_print("WARNING: Variable '" + var_name + "' has type mismatch: expected " +
                            (is_float ? "float" : "int") + " but got register " + allocation.assigned_register);
            }

            debug_print("Variable '" + var_name + "' is in register: " + allocation.assigned_register);
            return allocation.assigned_register;
        } else {
            // Check if the stack offset is valid or still pending
            if (allocation.stack_offset != -1) {
                debug_print("Variable '" + var_name + "' is spilled. Reloading from stack offset: " + std::to_string(allocation.stack_offset));

                // Determine if this is a float variable
                bool is_float = false;
                if (found_symbol) {
                    is_float = (symbol.type == VarType::FLOAT);
                    debug_print("Variable '" + var_name + "' type determined from symbol table: " +
                               (is_float ? "float" : "int"));
                } else {
                    // Fall back to ASTAnalyzer if symbol not found
                    VarType var_type = ASTAnalyzer::getInstance().get_variable_type(current_function_name_, var_name);
                    is_float = (var_type == VarType::FLOAT);
                    debug_print("Variable '" + var_name + "' type determined from ASTAnalyzer: " +
                               (is_float ? "float" : "int"));
                }

                if (current_frame_manager_) {
                    // Keep CallFrameManager in sync with our type info
                    if (is_float && !current_frame_manager_->is_float_variable(var_name)) {
                        current_frame_manager_->mark_variable_as_float(var_name);
                        debug_print("Updating CallFrameManager with float type for '" + var_name + "'");
                    } else if (!is_float && current_frame_manager_->is_float_variable(var_name)) {
                        // Trust CallFrameManager's type info if it conflicts
                        debug_print("CallFrameManager indicates '" + var_name + "' is float, using that");
                        is_float = true;
                    }
                }

                if (is_float) {
                    // Try to get a real float variable register (D8-D15) instead of a scratch register
                    std::string reg = register_manager_.acquire_spillable_fp_temp_reg(*this);
                    emit(Encoder::create_ldr_fp_imm(reg, "X29", allocation.stack_offset));
                    register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                    debug_print("Loaded float variable '" + var_name + "' into " + reg + " (callee-saved if possible)");
                    return reg;
                } else {
                    // Try to use a real variable register instead of a scratch register when possible
                    std::string reg = register_manager_.acquire_spillable_temp_reg(*this);
                    emit(Encoder::create_ldr_imm(reg, "X29", allocation.stack_offset));
                    register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                    debug_print("Variable '" + var_name + "' value loaded into " + reg);
                    return reg;
                }
            } else {
                // Stack offset is still pending, use current_frame_manager to get the offset
                debug_print("Variable '" + var_name + "' is spilled but offset is pending. Using CallFrameManager::get_offset");
                try {
                    int offset = current_frame_manager_->get_offset(var_name);

                    // Determine if this is a float variable
                    bool is_float = false;
                    if (found_symbol) {
                        is_float = (symbol.type == VarType::FLOAT);
                        debug_print("Variable '" + var_name + "' type determined from symbol table: " +
                                  (is_float ? "float" : "int"));
                    } else {
                        // Fall back to ASTAnalyzer if symbol not found
                        VarType var_type = ASTAnalyzer::getInstance().get_variable_type(current_function_name_, var_name);
                        is_float = (var_type == VarType::FLOAT);
                        debug_print("Variable '" + var_name + "' type determined from ASTAnalyzer: " +
                                  (is_float ? "float" : "int"));
                    }

                    if (current_frame_manager_) {
                        // Keep CallFrameManager in sync with our type info
                        if (is_float && !current_frame_manager_->is_float_variable(var_name)) {
                            current_frame_manager_->mark_variable_as_float(var_name);
                            debug_print("Updating CallFrameManager with float type for '" + var_name + "'");
                        } else if (!is_float && current_frame_manager_->is_float_variable(var_name)) {
                            // Trust CallFrameManager's type info if it conflicts
                            debug_print("CallFrameManager indicates '" + var_name + "' is float, using that");
                            is_float = true;
                        }
                    }

                    if (is_float) {
                        // Try to get a real float variable register (D8-D15) instead of a scratch register
                        std::string reg = register_manager_.acquire_spillable_fp_temp_reg(*this);
                        emit(Encoder::create_ldr_fp_imm(reg, "X29", offset));
                        register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                        debug_print("Loaded float variable '" + var_name + "' into " + reg + " (callee-saved if possible)");
                        return reg;
                    } else {
                        // Try to use a real variable register instead of a scratch register when possible
                        std::string reg = register_manager_.acquire_spillable_temp_reg(*this);
                        emit(Encoder::create_ldr_imm(reg, "X29", offset));
                        register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                        debug_print("Variable '" + var_name + "' value loaded into " + reg);
                        return reg;
                    }
                } catch (const std::runtime_error& e) {
                    // Fall back to old register allocation method if we can't get the offset
                    debug_print("WARNING: Could not get stack offset for spilled variable '" + var_name + "'. Using legacy allocation: " + std::string(e.what()));
                }
            }
        }
    }

    // If the symbol exists in the symbol table, check its location
    if (found_symbol) {
        debug_print("Found symbol in symbol table: " + var_name);

        if (symbol.location.type == SymbolLocation::LocationType::STACK) {
            // Variable is on the stack
            int offset = symbol.location.stack_offset;
            bool is_float = (symbol.type == VarType::FLOAT);
            debug_print("Symbol '" + var_name + "' is on stack at offset " + std::to_string(offset) +
                      ", type is " + (is_float ? "float" : "int"));

            if (is_float) {
                std::string reg = register_manager_.acquire_spillable_fp_temp_reg(*this);
                emit(Encoder::create_ldr_fp_imm(reg, "X29", offset));
                register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                return reg;
            } else {
                std::string reg = register_manager_.acquire_spillable_temp_reg(*this);
                emit(Encoder::create_ldr_imm(reg, "X29", offset));
                register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                return reg;
            }
        }
        else if (symbol.location.type == SymbolLocation::LocationType::DATA) {
            // Variable is in the data segment
            size_t offset = symbol.location.data_offset;
            bool is_float = (symbol.type == VarType::FLOAT);
            debug_print("Symbol '" + var_name + "' is in data segment at offset " + std::to_string(offset) +
                      ", type is " + (is_float ? "float" : "int"));

            if (is_float) {
                std::string reg = register_manager_.acquire_spillable_fp_temp_reg(*this);
                emit(Encoder::create_ldr_fp_imm(reg, "X28", offset * 8));
                register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                return reg;
            } else {
                std::string reg = register_manager_.acquire_spillable_temp_reg(*this);
                emit(Encoder::create_ldr_imm(reg, "X28", offset * 8));
                register_manager_.mark_dirty(reg, false); // Mark register as clean after load
                return reg;
            }
        }
        else if (symbol.location.type == SymbolLocation::LocationType::ABSOLUTE) {
            // Manifest constant with a fixed value
            int64_t value = symbol.location.absolute_value;
            debug_print("Symbol '" + var_name + "' is a manifest constant with value " + std::to_string(value));

            std::string reg = register_manager_.acquire_spillable_temp_reg(*this);
            emit(Encoder::create_movz_movk_abs64(reg, value, ""));
            return reg;
        }
    }

    // Fall back to global variables if not found in symbol table
    if (data_generator_.is_global_variable(var_name)) {
        auto& rm = register_manager_;
        std::string reg;
        VarType var_type = VarType::INTEGER;

        // Determine if this is a float variable
        if (found_symbol) {
            var_type = symbol.type;
        } else {
            // Fall back to ASTAnalyzer
            var_type = ASTAnalyzer::getInstance().get_variable_type(current_scope_name_, var_name);
        }

        if (var_type == VarType::FLOAT) {
            // Use a variable register (D8-D15) when possible instead of scratch
            reg = rm.acquire_spillable_fp_temp_reg(*this);
            size_t byte_offset = data_generator_.get_global_word_offset(var_name) * 8;
            emit(Encoder::create_ldr_fp_imm(reg, "X28", byte_offset));
            debug_print("Loaded global variable '" + var_name + "' into " + reg + " from X28 + #" + std::to_string(byte_offset));
        } else {
            // Use a variable register when possible instead of scratch
            reg = rm.acquire_spillable_temp_reg(*this);
            size_t byte_offset = data_generator_.get_global_word_offset(var_name) * 8;
            emit(Encoder::create_ldr_imm(reg, "X28", byte_offset));
            debug_print("Loaded global variable '" + var_name + "' into " + reg + " from X28 + #" + std::to_string(byte_offset));
        }
        return reg;
    }

    // Handle global functions (loading their address) (local?)
    if (ASTAnalyzer::getInstance().is_local_function(var_name)) {
        std::string reg = register_manager_.get_free_register();
        emit(Encoder::create_adrp(reg, var_name));
        emit(Encoder::create_add_literal(reg, reg, var_name));
        debug_print("Loaded address of global function '" + var_name + "' into " + reg + ".");
        return reg;
    }

    throw std::runtime_error("Variable '" + var_name + "' not found in any scope or as a global/static label.");
}


// Stores a register's value into a variable's memory location (stack for locals, data segment for globals).
void NewCodeGenerator::store_variable_register(const std::string& var_name, const std::string& reg_to_store) {
    // Look up the symbol in our symbol table if available
    Symbol symbol;
    bool found_symbol = lookup_symbol(var_name, symbol);

    // Skip runtime functions that shouldn't be stored
    if (found_symbol && symbol.is_runtime()) {
        // Runtime functions are read-only, no need to store
        return;
    }

    // Check if it's a local variable managed by the allocator
    auto it = current_function_allocation_.find(var_name);
    if (it != current_function_allocation_.end()) {
        const LiveInterval& allocation = it->second;
        if (!allocation.is_spilled) {
            debug_print("Storing variable '" + var_name + "' to assigned register: " + allocation.assigned_register);
            if (reg_to_store != allocation.assigned_register) {
                // Check if we're dealing with floating point registers
                bool src_is_fp = register_manager_.is_fp_register(reg_to_store);
                bool dst_is_fp = register_manager_.is_fp_register(allocation.assigned_register);

                if (src_is_fp && dst_is_fp) {
                    debug_print("Using fmov for float->float: " + reg_to_store + " to " + allocation.assigned_register);
                    emit(Encoder::create_fmov_reg(allocation.assigned_register, reg_to_store));
                } else if (!src_is_fp && !dst_is_fp) {
                    debug_print("Using mov for int->int: " + reg_to_store + " to " + allocation.assigned_register);
                    emit(Encoder::create_mov_reg(allocation.assigned_register, reg_to_store));
                } else if (!src_is_fp && dst_is_fp) {
                    debug_print("Converting int->float: " + reg_to_store + " to " + allocation.assigned_register);
                    emit(Encoder::create_scvtf_reg(allocation.assigned_register, reg_to_store));
                } else { // src_is_fp && !dst_is_fp
                    debug_print("Converting float->int: " + reg_to_store + " to " + allocation.assigned_register);
                    emit(Encoder::create_fcvtzs_reg(allocation.assigned_register, reg_to_store));
                }
            }
        } else {
            // Check if the stack offset is valid or still pending
            if (allocation.stack_offset != -1) {
                debug_print("Storing variable '" + var_name + "' to stack offset: " + std::to_string(allocation.stack_offset));

                // Check if this is a float variable or if the value is in a float register
                bool is_float = register_manager_.is_fp_register(reg_to_store);
                if (!is_float) {
                    // First check symbol table if available
                    if (found_symbol) {
                        is_float = (symbol.type == VarType::FLOAT);
                        debug_print("Variable '" + var_name + "' type from symbol table: " +
                                   (is_float ? "float" : "int"));
                    } else {
                        // Fall back to AST analyzer
                        VarType var_type = ASTAnalyzer::getInstance().get_variable_type(current_function_name_, var_name);
                        is_float = (var_type == VarType::FLOAT);
                    }
                    // Also check CallFrameManager
                    if (!is_float && current_frame_manager_ && current_frame_manager_->is_float_variable(var_name)) {
                        is_float = true;
                        debug_print("Variable '" + var_name + "' identified as float from CallFrameManager");
                    }
                } else {
                    // If storing a float value, update CallFrameManager
                    if (current_frame_manager_ && !current_frame_manager_->is_float_variable(var_name)) {
                        current_frame_manager_->mark_variable_as_float(var_name);
                        debug_print("Marked variable '" + var_name + "' as float in CallFrameManager due to float value assignment");
                    }
                }

                // Store with the appropriate instruction based on type
                if (is_float) {
                    emit(Encoder::create_str_fp_imm(reg_to_store, "X29", allocation.stack_offset));
                    debug_print("Storing float value from " + reg_to_store + " to stack offset " +
                               std::to_string(allocation.stack_offset) + " for '" + var_name + "'");
                } else {
                    emit(Encoder::create_str_imm(reg_to_store, "X29", allocation.stack_offset));
                    debug_print("Storing integer value from " + reg_to_store + " to stack offset " +
                               std::to_string(allocation.stack_offset) + " for '" + var_name + "'");
                }
            } else {
                // Stack offset is still pending, use current_frame_manager to get the offset
                debug_print("Variable '" + var_name + "' is spilled but offset is pending. Using CallFrameManager::get_offset");
                try {
                    int offset = current_frame_manager_->get_offset(var_name);

                    // Check if this is a float variable or if the value is in a float register
                    bool is_float = register_manager_.is_fp_register(reg_to_store);
                    if (!is_float) {
                        // First check symbol table if available
                        if (found_symbol) {
                            is_float = (symbol.type == VarType::FLOAT);
                            debug_print("Variable '" + var_name + "' type from symbol table: " +
                                       (is_float ? "float" : "int"));
                        } else {
                            // Fall back to AST analyzer
                            VarType var_type = ASTAnalyzer::getInstance().get_variable_type(current_function_name_, var_name);
                            is_float = (var_type == VarType::FLOAT);
                        }
                        // Also check CallFrameManager
                        if (!is_float && current_frame_manager_ && current_frame_manager_->is_float_variable(var_name)) {
                            is_float = true;
                            debug_print("Variable '" + var_name + "' identified as float from CallFrameManager");
                        }
                    } else {
                        // If storing a float value, update CallFrameManager
                        if (current_frame_manager_ && !current_frame_manager_->is_float_variable(var_name)) {
                            current_frame_manager_->mark_variable_as_float(var_name);
                            debug_print("Marked variable '" + var_name + "' as float in CallFrameManager due to float value assignment");
                        }
                    }

                    if (is_float) {
                        emit(Encoder::create_str_fp_imm(reg_to_store, "X29", offset));
                    } else {
                        emit(Encoder::create_str_imm(reg_to_store, "X29", offset));
                    }

                    // Update the allocation with the now-known offset
                    const_cast<LiveInterval&>(allocation).stack_offset = offset;
                } catch (const std::runtime_error& e) {
                    // Fall back to old variable storage method if we can't get the offset
                    debug_print("WARNING: Could not get stack offset for spilled variable '" + var_name + "'. Using legacy storage: " + std::string(e.what()));
                    // Try to continue with the next method (which may or may not work)
                }
            }
        }
        return; // Finished
    }

    // If this variable is not managed by our allocator yet, add it
    if (current_frame_manager_ && current_frame_manager_->has_local(var_name)) {
        debug_print("Adding variable '" + var_name + "' to allocation map (direct store)");
        LiveInterval new_interval;
        new_interval.var_name = var_name;
        new_interval.start_point = 0;  // Conservative estimate
        new_interval.end_point = 1000; // Conservative estimate

        // Try to get an available register
        if (!RegisterManager::VARIABLE_REGS.empty()) {
            new_interval.assigned_register = reg_to_store; // Use the provided register
            new_interval.is_spilled = false;
            current_function_allocation_[var_name] = new_interval;
            debug_print("Assigned register " + reg_to_store + " to variable '" + var_name + "'");
            return;
        } else {
            // Need to spill
            new_interval.is_spilled = true;
            try {
                new_interval.stack_offset = current_frame_manager_->get_offset(var_name);
                current_function_allocation_[var_name] = new_interval;
                // Check if this is a float variable or if the value is in a float register
                bool is_float = register_manager_.is_fp_register(reg_to_store);
                if (!is_float) {
                    // First check symbol table if available
                    if (found_symbol) {
                        is_float = (symbol.type == VarType::FLOAT);
                        debug_print("Variable '" + var_name + "' type from symbol table: " +
                                   (is_float ? "float" : "int"));
                    } else {
                        // Fall back to AST analyzer
                        VarType var_type = ASTAnalyzer::getInstance().get_variable_type(current_function_name_, var_name);
                        is_float = (var_type == VarType::FLOAT);
                    }
                    // Also check CallFrameManager
                    if (!is_float && current_frame_manager_ && current_frame_manager_->is_float_variable(var_name)) {
                        is_float = true;
                        debug_print("Variable '" + var_name + "' identified as float from CallFrameManager");
                    }
                } else {
                    // If storing a float value, update CallFrameManager
                    if (current_frame_manager_ && !current_frame_manager_->is_float_variable(var_name)) {
                        current_frame_manager_->mark_variable_as_float(var_name);
                        debug_print("Marked variable '" + var_name + "' as float in CallFrameManager due to float value assignment");
                    }
                }

                if (is_float || register_manager_.is_fp_register(reg_to_store)) {
                    // If variable is float or value is in float register
                    emit(Encoder::create_str_fp_imm(reg_to_store, "X29", new_interval.stack_offset));
                    debug_print("Spilled float variable '" + var_name + "' to stack offset: " + std::to_string(new_interval.stack_offset));
                } else {
                    emit(Encoder::create_str_imm(reg_to_store, "X29", new_interval.stack_offset));
                    debug_print("Spilled variable '" + var_name + "' to stack offset: " + std::to_string(new_interval.stack_offset));
                }
                return;
            } catch (const std::runtime_error& e) {
                debug_print("WARNING: Failed to get offset for variable '" + var_name + "': " + std::string(e.what()));
            }
        }
    }

    // Check if symbol exists in the symbol table and is a global/static variable
    if (found_symbol && (symbol.is_global())) {
        if (symbol.location.type == SymbolLocation::LocationType::DATA) {
            size_t byte_offset = symbol.location.data_offset * 8;
            bool is_float = (symbol.type == VarType::FLOAT);

            if (is_float || register_manager_.is_fp_register(reg_to_store)) {
                emit(Encoder::create_str_fp_imm(reg_to_store, "X28", byte_offset));
                debug_print("Stored float value to global '" + var_name + "' at X28+" + std::to_string(byte_offset));
            } else {
                emit(Encoder::create_str_imm(reg_to_store, "X28", byte_offset));
                debug_print("Stored integer value to global '" + var_name + "' at X28+" + std::to_string(byte_offset));
            }
            return;
        }
    }

    // Fall back to legacy global variable check
    if (data_generator_.is_global_variable(var_name)) {
        size_t byte_offset = data_generator_.get_global_word_offset(var_name) * 8;

        // Check type using symbol table if available
        bool is_float = register_manager_.is_fp_register(reg_to_store);
        if (!is_float && found_symbol) {
            is_float = (symbol.type == VarType::FLOAT);
        }

        if (is_float || register_manager_.is_fp_register(reg_to_store)) {
            emit(Encoder::create_str_fp_imm(reg_to_store, "X28", byte_offset));
        } else {
            emit(Encoder::create_str_imm(reg_to_store, "X28", byte_offset));

        }

        debug_print("Stored register " + reg_to_store + " into global variable '" + var_name + "' at X28 + #" + std::to_string(byte_offset));
        return;
    }

    // If no match, it's an error.
    throw std::runtime_error("Cannot store to variable '" + var_name + "': not found as local or global.");
}


// --- CORE: generate_function_like_code implementation ---
// This helper encapsulates the common logic for generating prologues, epilogues,
// and managing the call frame for functions and routines.
void NewCodeGenerator::generate_function_like_code(
    const std::string& name,
    const std::vector<std::string>& parameters, // Function/Routine parameters
    ASTNode& body_node,
    bool is_function_returning_value
) {
    current_function_name_ = name; // Track the current function name
    current_function_return_type_ = ASTAnalyzer::getInstance().get_function_return_types().at(name); // Retrieve and store the function's return type
    current_scope_name_ = name;    // Initialize scope tracking
    block_id_counter_ = 0;         // Reset block ID counter for each new function
    debug_print("DEBUG: generate_function_like_code called for: " + name);
    debug_print("Generating function-like code for: " + name);
    x28_is_loaded_in_current_function_ = false;

    // Create and store the unique epilogue label for this function
    current_function_epilogue_label_ = label_manager_.create_label();

    // Define the entry point label for the function/routine.
    instruction_stream_.define_label(name);

    // Set up a new CallFrameManager for this function/routine.
    current_frame_manager_ = std::make_unique<CallFrameManager>(register_manager_, name, debug_enabled_);

    // Force saving of X19 and X20 because they are used as routine cache registers
    // and their values need to persist across calls within this function.
    current_frame_manager_->force_save_x19_x20();

    // Inform the CallFrameManager about the parameters BEFORE generating the prologue.
    for (const auto& param_name : parameters) {
        current_frame_manager_->add_parameter(param_name);
    }

    // START OF THE FIX
    // =======================================================================
    // Recursively find and register all local variables from LET declarations
    // within the function's body BEFORE generating the prologue.

    std::function<void(ASTNode*)> find_and_register_locals =
        [&](ASTNode* node) {
        if (!node) return;

        // If this node is a LetDeclaration, register its variables.
        if (auto* let_decl = dynamic_cast<LetDeclaration*>(node)) {
            for (const auto& var_name : let_decl->names) {
                // Check if it's not a parameter and not already added.
                if (std::find(parameters.begin(), parameters.end(), var_name) == parameters.end() &&
                    !current_frame_manager_->has_local(var_name)) {

                    // We don't distinguish size here; assume all locals are 8 bytes.
                    // Only add to stack if not a global
                    Symbol symbol;
                    if (!symbol_table_ || !symbol_table_->lookup(var_name, symbol) || !symbol.is_global()) {
                        current_frame_manager_->add_local(var_name, 8);
                        debug_print("Pre-registered VALOF/local variable '" + var_name + "' for '" + name + "'.");
                    } else {
                        debug_print("Recognized '" + var_name + "' as a global. It will not be allocated on the stack.");
                    }
                }
            }
        }

        // --- Recursively visit all children of the current node ---
        // This list must cover all node types that can contain declarations.
        if (auto* block = dynamic_cast<BlockStatement*>(node)) {
            for (auto& stmt : block->statements) find_and_register_locals(stmt.get());
        } else if (auto* compound = dynamic_cast<CompoundStatement*>(node)) {
            for (auto& stmt : compound->statements) find_and_register_locals(stmt.get());
        } else if (auto* valof = dynamic_cast<ValofExpression*>(node)) {
            find_and_register_locals(valof->body.get());
        } else if (auto* fvalof = dynamic_cast<FloatValofExpression*>(node)) {
            find_and_register_locals(fvalof->body.get());
        } else if (auto* if_stmt = dynamic_cast<IfStatement*>(node)) {
            find_and_register_locals(if_stmt->then_branch.get());
        } else if (auto* unless_stmt = dynamic_cast<UnlessStatement*>(node)) {
            find_and_register_locals(unless_stmt->then_branch.get());
        } else if (auto* test_stmt = dynamic_cast<TestStatement*>(node)) {
            find_and_register_locals(test_stmt->then_branch.get());
            find_and_register_locals(test_stmt->else_branch.get());
        } else if (auto* for_stmt = dynamic_cast<ForStatement*>(node)) {
            find_and_register_locals(for_stmt->start_expr.get());
            find_and_register_locals(for_stmt->end_expr.get());
            find_and_register_locals(for_stmt->step_expr.get());
            find_and_register_locals(for_stmt->body.get());
        } else if (auto* while_stmt = dynamic_cast<WhileStatement*>(node)) {
            find_and_register_locals(while_stmt->condition.get());
            find_and_register_locals(while_stmt->body.get());
        } else if (auto* until_stmt = dynamic_cast<UntilStatement*>(node)) {
            find_and_register_locals(until_stmt->condition.get());
            find_and_register_locals(until_stmt->body.get());
        } else if (auto* repeat_stmt = dynamic_cast<RepeatStatement*>(node)) {
            find_and_register_locals(repeat_stmt->body.get());
            find_and_register_locals(repeat_stmt->condition.get());
        }
    };

    // Start the recursive search from the function's body.
    find_and_register_locals(&body_node);

    // =======================================================================
    // END OF THE FIX
    enter_scope();

    // Pre-register locals/parameters for this entity using ASTAnalyzer's pre-scan results.
    std::unordered_set<std::string> param_set(parameters.begin(), parameters.end()); // For quick lookup

    for (const auto& pair : ASTAnalyzer::getInstance().get_variable_definitions()) {
        // 'pair.first' is the unique variable name (e.g., "Y", "I_for_loop_0")
        // 'pair.second' is the scope name (e.g., "START_block_0")
        if ((pair.second == name || pair.second.find(name + "_block_") == 0) &&
            param_set.find(pair.first) == param_set.end()) { // If not a parameter
            // Only add to stack if not a global
            Symbol symbol;
            if (!symbol_table_ || !symbol_table_->lookup(pair.first, symbol) || !symbol.is_global()) {
                current_frame_manager_->add_local(pair.first, 8); // Add to CFM for stack space
                current_scope_symbols_[pair.first] = -1; // Mark as local in symbol table (offset from CFM)
                debug_print("Pre-registered local variable '" + pair.first + "' for '" + name + "'.");
            } else {
                debug_print("Recognized '" + pair.first + "' as a global. It will not be allocated on the stack.");
            }
        } else if (param_set.find(pair.first) != param_set.end()) {
            current_scope_symbols_[pair.first] = -1; // Mark parameter in symbol table as local
            debug_print("Pre-registered parameter '" + pair.first + "' as local for '" + name + "'.");
        }
    }

    // --- HEURISTIC-BASED SPILL SLOT RESERVATION ---
    int max_live = 0;
    auto& metrics_map = ASTAnalyzer::getInstance().get_function_metrics();
    auto it = metrics_map.find(name);
    if (it != metrics_map.end()) {
        max_live = it->second.max_live_variables;
    }
    int available_regs = RegisterManager::VARIABLE_REGS.size();
    if (max_live > available_regs) {
        int needed_spills = max_live - available_regs;
        debug_print("Heuristic: Reserving " + std::to_string(needed_spills) + " spill slots for '" + name + "'.");
        current_frame_manager_->preallocate_spill_slots(needed_spills);
    }

    // --- Reserve callee-saved registers based on register pressure ---
    current_frame_manager_->reserve_registers_based_on_pressure(max_live);

    // Generate the function/routine prologue.
    debug_print("Attempting to generate prologue for '" + name + "'.");
    for (const auto& instr : current_frame_manager_->generate_prologue()) {
        emit(instr);
    }

    // Update stack offsets for spilled variables now that the prologue has been generated
    update_spill_offsets();

    // Register all local variables in the current frame with the allocation system
    for (const auto& local_name : current_frame_manager_->get_local_variable_names()) {
        if (current_function_allocation_.find(local_name) == current_function_allocation_.end()) {
            // Create a new interval for this variable
            LiveInterval new_interval;
            new_interval.var_name = local_name;
            new_interval.start_point = 0;  // Conservative estimate
            new_interval.end_point = 1000; // Conservative estimate
            new_interval.is_spilled = true; // Default to spilled

            try {
                new_interval.stack_offset = current_frame_manager_->get_offset(local_name);
                current_function_allocation_[local_name] = new_interval;
                debug_print("Registered local variable '" + local_name + "' with the allocation system (spilled)");
            } catch (const std::runtime_error& e) {
                debug_print("WARNING: Failed to register local variable '" + local_name + "': " + std::string(e.what()));
            }
        }
    }

    // If this function accesses global variables, emit a MOVZ/MOVK sequence
    // to load the absolute 64-bit address of the data segment base into X28.
    // The immediate values are patched by the linker.
    if (ASTAnalyzer::getInstance().function_accesses_globals(name)) {
        if (data_segment_base_addr_ == 0) {
            throw std::runtime_error("JIT mode requires a valid data_segment_base_addr.");
        }
        // Directly use the known 64-bit address. The symbol is now empty as no relocation is needed.
        emit(Encoder::create_movz_movk_jit_addr("X28", data_segment_base_addr_, "L__data_segment_base"));
        x28_is_loaded_in_current_function_ = true;
        debug_print("Directly emitted JIT address load sequence for X28.");
    }

    // Store incoming argument registers into their stack slots.
    for (size_t i = 0; i < parameters.size(); ++i) {
        const std::string& param_name = parameters[i];
        int offset = current_frame_manager_->get_offset(param_name);

        // --- REVISED FIX IS HERE ---
        // The decision is now based on the function's return type, not the parameter's type.
        if (current_function_return_type_ == VarType::FLOAT) {
            // This is a float function, so assume the argument was passed in a D register.
            std::string arg_reg = "D" + std::to_string(i);
            emit(Encoder::create_str_fp_imm(arg_reg, "X29", offset));
        } else {
            // This is an integer function, so assume the argument was passed in an X register.
            std::string arg_reg = "X" + std::to_string(i);
            emit(Encoder::create_str_imm(arg_reg, "X29", offset, param_name));
        }
        // --- END REVISED FIX ---
    }
    debug_print(current_frame_manager_->display_frame_layout());

    // --- CFG-DRIVEN CODE GENERATION LOOP ---
    // Look up the CFG for this function
    const auto& cfgs = cfg_builder_.get_cfgs();
    auto cfg_it = cfgs.find(name);
    if (cfg_it == cfgs.end()) {
        throw std::runtime_error("Could not find CFG for function: " + name);
    }
    const ControlFlowGraph* cfg = cfg_it->second.get();

    // Create a sorted list of blocks for deterministic code output
    std::vector<BasicBlock*> blocks;
    for (const auto& pair : cfg->get_blocks()) {
        blocks.push_back(pair.second.get());
    }
    std::sort(blocks.begin(), blocks.end(), [](auto* a, auto* b) {
        // Entry block should always come first
        if (a->is_entry) return true;
        if (b->is_entry) return false;
        // Otherwise sort alphabetically by ID
        return a->id < b->id;
    });

    // --- MAIN CODE GENERATION LOOP ---
    for (BasicBlock* block : blocks) {
        // Every basic block starts with a label
        instruction_stream_.define_label(block->id);

        // Generate code for each statement within the block
        for (const auto& stmt : block->statements) {
            stmt->accept(*this);
        }

        // Generate the branching logic to connect this block to its successors
        generate_block_epilogue(block);
    }
    // --- END OF CFG-DRIVEN LOOP ---

    // --- NEW: Define the shared function exit point here ---
    debug_print("Defining epilogue label: " + current_function_epilogue_label_);
    instruction_stream_.define_label(current_function_epilogue_label_);

    debug_print("Attempting to generate epilogue for '" + name + "'.");
    emit(current_frame_manager_->generate_epilogue());

    // Exit the scope
    exit_scope();

    // Clean up the CallFrameManager unique_ptr.
    register_manager_.clear_routine_cache();
    current_frame_manager_.reset();
}

// --- CFG-driven codegen: block epilogue logic ---
void NewCodeGenerator::generate_block_epilogue(BasicBlock* block) {
    if (block->successors.empty()) {
        // This block ends with a RETURN, FINISH, etc. which should have already
        // emitted a branch to the main epilogue. If not, it's a fallthrough to the end.
        if (!block->ends_with_control_flow()) {
            emit(Encoder::create_branch_unconditional(current_function_epilogue_label_));
        }
        return;
    }

    else if (block->successors.size() == 1) {
        // Check if this is a block that ends with a LoopStatement
        const Statement* last_stmt = block->statements.empty() ? nullptr : block->statements.back().get();

        if (dynamic_cast<const LoopStatement*>(last_stmt)) {
            // Generate an unconditional branch to the loop's start (the successor)
            debug_print("Generating branch for LOOP statement based on CFG");
            if (debug_enabled_) {
                std::cerr << "DEBUG: LOOP codegen - Emitting branch from block " << block->id
                          << " to target " << block->successors[0]->id << "\n";
            }
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
            return;
        }

        // For other blocks with one successor, generate unconditional branch
        // Before generating an unconditional branch, check if this is an assignment block that follows a loop condition
        // This helps ensure assignments like increments get processed properly
        bool is_increment_block = false;
        if (!block->statements.empty()) {
            // Check if this block contains exactly one statement which is an assignment
            if (block->statements.size() == 1 && block->statements[0]->getType() == ASTNode::NodeType::AssignmentStmt) {
                // Check if any predecessor is a loop block
                for (const auto* pred : block->predecessors) {
                    if (!pred->statements.empty()) {
                        const auto* last_stmt = pred->statements.back().get();
                        if (dynamic_cast<const ForStatement*>(last_stmt) ||
                            dynamic_cast<const WhileStatement*>(last_stmt) ||
                            dynamic_cast<const UntilStatement*>(last_stmt) ||
                            dynamic_cast<const RepeatStatement*>(last_stmt)) {
                            is_increment_block = true;
                            if (debug_enabled_) {
                                std::cerr << "DEBUG: Found increment block " << block->id
                                          << " with predecessor " << pred->id << " (loop type)\n";
                            }
                            break;
                        }
                    }
                }
            }
        }

        if (is_increment_block) {
            if (debug_enabled_) {
                std::cerr << "DEBUG: Processing increment block " << block->id
                          << " with " << block->statements.size() << " statements\n";
            }

            // Process the statement(s) in this block before branching
            for (const auto& stmt : block->statements) {
                if (debug_enabled_) {
                    std::cerr << "DEBUG: Processing statement in increment block "
                                  << "\n";
                }
                stmt->accept(*this);
            }

            if (debug_enabled_) {
                std::cerr << "DEBUG: Finished processing increment block statements\n";
            }
        } else {
            if (debug_enabled_) {
                std::cerr << "DEBUG: Block " << block->id
                          << " has one successor but is NOT detected as an increment block\n";
            }
        }

        emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        return;
    }

    else if (block->successors.size() == 2) {
        // Conditional branch. The last statement in the block determines the condition.
        const Statement* last_stmt = block->statements.empty() ? nullptr : block->statements.back().get();

        if (const auto* if_stmt = dynamic_cast<const IfStatement*>(last_stmt)) {
            // Handle IF statement: successors[0] is 'then' block, successors[1] is 'else' or join block.
            generate_expression_code(*if_stmt->condition);
            std::string cond_reg = expression_result_reg_;
            emit(Encoder::create_cmp_reg(cond_reg, "XZR")); // is condition false?
            register_manager_.release_register(cond_reg);

            // If condition is FALSE (equal to zero), jump to the join/else block (second successor).
            emit(Encoder::create_branch_conditional("EQ", block->successors[1]->id));
            // Otherwise, fall through to the unconditional branch to the THEN block (first successor).
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        } else if (const auto* unless_stmt = dynamic_cast<const UnlessStatement*>(last_stmt)) {
            // Handle UNLESS statement: successors[0] is 'then' block (executed when condition is false),
            // successors[1] is join block (executed when condition is true).
            generate_expression_code(*unless_stmt->condition);
            std::string cond_reg = expression_result_reg_;
            emit(Encoder::create_cmp_reg(cond_reg, "XZR")); // is condition false?
            register_manager_.release_register(cond_reg);

            // If condition is TRUE (not equal to zero), jump to the second successor.
            emit(Encoder::create_branch_conditional("NE", block->successors[1]->id));
            // Otherwise, fall through to the unconditional branch to the first successor.
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        } else if (const auto* test_stmt = dynamic_cast<const TestStatement*>(last_stmt)) {
            // Handle TEST statement: successors[0] is 'then' block, successors[1] is 'else' block.
            generate_expression_code(*test_stmt->condition);
            std::string cond_reg = expression_result_reg_;
            emit(Encoder::create_cmp_reg(cond_reg, "XZR")); // is condition false?
            register_manager_.release_register(cond_reg);

            // If condition is FALSE (equal to zero), jump to the second successor.
            emit(Encoder::create_branch_conditional("EQ", block->successors[1]->id));
            // Otherwise, fall through to the unconditional branch to the first successor.
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        } else if (const auto* cond_branch_stmt = dynamic_cast<const ConditionalBranchStatement*>(last_stmt)) {
            // Handle ConditionalBranchStatement: this is already a low-level conditional branch
            // The ConditionalBranchStatement should have already been processed by its visitor
            // and emitted the appropriate conditional branch instruction.
            // For blocks with two successors, we need to emit the conditional branch here.
            generate_expression_code(*cond_branch_stmt->condition_expr);
            std::string cond_reg = expression_result_reg_;
            emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
            register_manager_.release_register(cond_reg);

            // Emit conditional branch using the condition and target from ConditionalBranchStatement
            emit(Encoder::create_branch_conditional(cond_branch_stmt->condition, block->successors[1]->id));
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        } else if (const auto* for_stmt = dynamic_cast<const ForStatement*>(last_stmt)) {
            // Handle ForStatement: evaluate loop condition (loop_var <= end_expr)
            // Generate code to check: loop_variable <= end_expr
            // If true, continue to loop body (first successor)
            // If false, exit loop (second successor)

            if (debug_enabled_) {
                std::cerr << "DEBUG: Generating ForStatement condition check in block " << block->id << "\n";
                std::cerr << "DEBUG: Block has " << block->successors.size() << " successors\n";
                for (size_t i = 0; i < block->successors.size(); ++i) {
                    std::cerr << "DEBUG:   Successor[" << i << "]: " << block->successors[i]->id << "\n";
                }
            }

            // Load current loop variable value
            std::string loop_var_reg = register_manager_.acquire_scratch_reg();
            emit(Encoder::create_ldr_imm(loop_var_reg, "X29", current_frame_manager_->get_offset(for_stmt->unique_loop_variable_name)));

            // Evaluate end expression
            generate_expression_code(*for_stmt->end_expr);
            std::string end_reg = expression_result_reg_;

            // Compare loop_var with end_value
            emit(Encoder::create_cmp_reg(loop_var_reg, end_reg));
            register_manager_.release_register(loop_var_reg);
            register_manager_.release_register(end_reg);

            // If loop_var > end_value, exit loop (branch to second successor)
            emit(Encoder::create_branch_conditional("GT", block->successors[1]->id));

            // NOTE: Increment code is intentionally removed here to avoid double increments
            // The CFGBuilderPass already adds an increment statement to the loop body
            // which handles the loop counter increment correctly

            // Continue to loop body (first successor)
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        } else if (const auto* while_stmt = dynamic_cast<const WhileStatement*>(last_stmt)) {
            // Handle WhileStatement: evaluate loop condition
            // If true, continue to loop body (first successor)
            // If false, exit loop (second successor)

            generate_expression_code(*while_stmt->condition);
            std::string cond_reg = expression_result_reg_;
            emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
            register_manager_.release_register(cond_reg);

            // If condition is FALSE (equal to zero), exit loop (branch to second successor)
            emit(Encoder::create_branch_conditional("EQ", block->successors[1]->id));
            // Otherwise, continue to loop body (first successor)
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        } else if (const auto* until_stmt = dynamic_cast<const UntilStatement*>(last_stmt)) {
            // Handle UntilStatement: evaluate loop condition (opposite of while)
            // If false, continue to loop body (first successor)
            // If true, exit loop (second successor)

            generate_expression_code(*until_stmt->condition);
            std::string cond_reg = expression_result_reg_;
            emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
            register_manager_.release_register(cond_reg);

            // If condition is TRUE (not equal to zero), exit loop (branch to second successor)
            emit(Encoder::create_branch_conditional("NE", block->successors[1]->id));
            // Otherwise, continue to loop body (first successor)
            emit(Encoder::create_branch_unconditional(block->successors[0]->id));
        } else if (const auto* repeat_stmt = dynamic_cast<const RepeatStatement*>(last_stmt)) {
            // Handle RepeatStatement with conditions (REPEAT...WHILE or REPEAT...UNTIL)
            if (repeat_stmt->loop_type == RepeatStatement::LoopType::RepeatWhile) {
                // REPEAT <body> WHILE <condition>
                // If true, loop back (first successor), if false, exit (second successor)
                generate_expression_code(*repeat_stmt->condition);
                std::string cond_reg = expression_result_reg_;
                emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
                register_manager_.release_register(cond_reg);

                // If condition is FALSE, exit loop (branch to second successor)
                emit(Encoder::create_branch_conditional("EQ", block->successors[1]->id));
                // Otherwise, loop back (first successor)
                emit(Encoder::create_branch_unconditional(block->successors[0]->id));
            } else if (repeat_stmt->loop_type == RepeatStatement::LoopType::RepeatUntil) {
                // REPEAT <body> UNTIL <condition>
                // If true, exit (first successor), if false, loop back (second successor)
                generate_expression_code(*repeat_stmt->condition);
                std::string cond_reg = expression_result_reg_;
                emit(Encoder::create_cmp_reg(cond_reg, "XZR"));
                register_manager_.release_register(cond_reg);

                // For REPEAT UNTIL: condition TRUE -> exit loop, condition FALSE -> loop back
                // CFGBuilderPass adds edges in this order:
                // successors[0] = loop_exit (for TRUE condition)
                // successors[1] = loop_entry (for FALSE condition)

                // If condition is TRUE (not equal to zero), exit the loop (to successors[0])
                emit(Encoder::create_branch_conditional("NE", block->successors[0]->id));
                // Otherwise (if condition is FALSE), loop back (to successors[1])
                emit(Encoder::create_branch_unconditional(block->successors[1]->id));
            } else {
                // Simple REPEAT loop without condition (always loops back)
                emit(Encoder::create_branch_unconditional(block->successors[0]->id));
            }
        } else {
            std::string error_msg = "Block has two successors but last statement is not a recognized conditional.";
            if (last_stmt) {
                error_msg += " Last statement type: " + std::to_string(static_cast<int>(last_stmt->getType()));
                error_msg += " (";
                switch (last_stmt->getType()) {
                    case ASTNode::NodeType::IfStmt: error_msg += "IfStmt"; break;
                    case ASTNode::NodeType::UnlessStmt: error_msg += "UnlessStmt"; break;
                    case ASTNode::NodeType::TestStmt: error_msg += "TestStmt"; break;
                    case ASTNode::NodeType::ConditionalBranchStmt: error_msg += "ConditionalBranchStmt"; break;
                    case ASTNode::NodeType::LabelTargetStmt: error_msg += "LabelTargetStmt"; break;
                    case ASTNode::NodeType::AssignmentStmt: error_msg += "AssignmentStmt"; break;
                    case ASTNode::NodeType::RoutineCallStmt: error_msg += "RoutineCallStmt"; break;
                    case ASTNode::NodeType::WhileStmt: error_msg += "WhileStmt"; break;
                    case ASTNode::NodeType::ForStmt: error_msg += "ForStmt"; break;
                    case ASTNode::NodeType::UntilStmt: error_msg += "UntilStmt"; break;
                    case ASTNode::NodeType::RepeatStmt: error_msg += "RepeatStmt"; break;
                    default: error_msg += "Unknown"; break;
                }
                error_msg += ")";
            } else {
                error_msg += " Block has no statements.";
            }
            throw std::runtime_error(error_msg);
        }
        return;
    }
    // START of inserted fix
    else if (block->successors.size() > 2) {
        // The last statement in the block should be a SwitchonStatement
        const Statement* last_stmt = block->statements.empty() ? nullptr : block->statements.back().get();
        const SwitchonStatement* switchon = dynamic_cast<const SwitchonStatement*>(last_stmt);

        if (!switchon) {
            std::string error_msg = "Block with >2 successors expected to end with SwitchonStatement, but got ";
            if (last_stmt) {
                error_msg += std::to_string(static_cast<int>(last_stmt->getType()));
            } else {
                error_msg += "no statement";
            }
            throw std::runtime_error(error_msg);
        }

        // 1. Evaluate the switch expression. The result is assumed to be in a register
        // that is still live. For simplicity, we re-evaluate it here. A more optimized
        // approach would pass the result register from the previous block.
        generate_expression_code(*switchon->expression);
        std::string switch_reg = expression_result_reg_;

        // 2. For each CASE, emit a comparison and conditional branch.
        // The order of successors in the CFG must match the order of cases in the AST.
        size_t num_cases = switchon->cases.size();
        bool has_default = (switchon->default_case != nullptr);

        for (size_t i = 0; i < num_cases; ++i) {
            const auto& case_stmt = switchon->cases[i];
            if (!case_stmt->resolved_constant_value.has_value()) {
                throw std::runtime_error("CaseStatement missing resolved constant value during codegen.");
            }
            int64_t case_value = case_stmt->resolved_constant_value.value();

            // Load the case constant into a temp register
            std::string case_reg = register_manager_.acquire_scratch_reg();
            // Using MOVZ/MOVK for potentially large constants
            auto mov_instrs = Encoder::create_movz_movk_abs64(case_reg, case_value, "");
            emit(mov_instrs);

            // Compare switch_reg with case_reg
            emit(Encoder::create_cmp_reg(switch_reg, case_reg));
            register_manager_.release_register(case_reg);

            // If equal, branch to the corresponding case block
            emit(Encoder::create_branch_conditional("EQ", block->successors[i]->id));
        }

        // 3. If there is a DEFAULT, branch to it; otherwise, branch to the end_switch block.
        // The fallback successor is the last one in the list.
        emit(Encoder::create_branch_unconditional(block->successors.back()->id));

        register_manager_.release_register(switch_reg);
        return;
    }
    // END of inserted fix
}


// --- ASTVisitor Implementations ---
// These are declared in NewCodeGenerator.h but their implementations
// are located in separate files (e.g., generators/gen_Program.cpp).

// CFG-driven: IfStatement visitor only evaluates condition, branching is handled by block epilogue.
void NewCodeGenerator::visit(IfStatement& node) {
    debug_print("Visiting IfStatement node (NOTE: branching is handled by block epilogue).");
    // No branching logic here; only evaluate condition if needed elsewhere.
}
// They are NOT included here to avoid duplication.
//
// Example (conceptual, actual code is in respective gen_*.cpp files):
//
// void NewCodeGenerator::visit(Program& node) { /* ... in gen_Program.cpp ... */ }
// void NewCodeGenerator::visit(LetDeclaration& node) { /* ... in gen_LetDeclaration.cpp ... */ }
// ... (all other visit methods) ...

void NewCodeGenerator::visit(GlobalVariableDeclaration& node) {
    debug_print("Visiting GlobalVariableDeclaration node for: " + node.names[0]);
    for (size_t i = 0; i < node.names.size(); ++i) {
        const auto& name = node.names[i];
        const auto* initializer_ptr = (i < node.initializers.size()) ? node.initializers[i].get() : nullptr;

        // Add the global variable to the data generator to reserve space
        // and store its initial value.
        if (initializer_ptr) {
            data_generator_.add_global_variable(name, clone_unique_ptr(node.initializers[i]));
        } else {
            data_generator_.add_global_variable(name, nullptr);
        }
        debug_print("Registered global variable '" + name + "' with the DataGenerator.");
    }
}
