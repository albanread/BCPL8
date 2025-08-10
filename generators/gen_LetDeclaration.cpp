#include "../NewCodeGenerator.h"
#include "../AST.h"
#include <stdexcept>



// --- Public Method ---

void NewCodeGenerator::visit(LetDeclaration& node) {
    debug_print("Visiting LetDeclaration node.");
    debug_print_level(
        std::string("Visiting LetDeclaration: ") +
        (current_frame_manager_ ? " (local)" : " (global)") +
        " at " + std::to_string(reinterpret_cast<uintptr_t>(&node)),
        4);

    if (node.names.size() != node.initializers.size()) {
        throw std::runtime_error("LetDeclaration: Mismatch between names and initializers.");
    }

    for (size_t i = 0; i < node.names.size(); ++i) {
        const std::string& name = node.names[i];
        ExprPtr& initializer = node.initializers[i];

        // Scenario 1: Function-like declaration (VALOF block)
        if (is_function_like_declaration(initializer)) {
            handle_function_like_declaration(name, initializer);
            continue;
        }

        // Scenario 2: Local variable declaration
        if (current_frame_manager_) {
            handle_local_variable_declaration(name, node.is_float_declaration);
            if (initializer) {
                // --- THIS IS THE FIX ---
                // On the fly, create an AssignmentStatement AST node from the LetDeclaration
                auto lhs = std::make_unique<VariableAccess>(name);
                std::vector<ExprPtr> lhs_vec;
                lhs_vec.push_back(std::move(lhs));

                std::vector<ExprPtr> rhs_vec;
                // Use a clone of the initializer since we may still need it elsewhere
                rhs_vec.push_back(clone_unique_ptr<Expression>(initializer));

                auto assignment = std::make_unique<AssignmentStatement>(std::move(lhs_vec), std::move(rhs_vec));

                // Now, visit the newly created AssignmentStatement to generate its code
                assignment->accept(*this);
                // --- END OF FIX ---
            }
        }
        // Scenario 3: Global variable or VALOF block variable
        else {
            if (is_valof_block_variable(name)) {
                handle_valof_block_variable(name);
                if (initializer) {
                    // For VALOF block variables, also generate assignment code
                    auto lhs = std::make_unique<VariableAccess>(name);
                    std::vector<ExprPtr> lhs_vec;
                    lhs_vec.push_back(std::move(lhs));

                    std::vector<ExprPtr> rhs_vec;
                    rhs_vec.push_back(clone_unique_ptr<Expression>(initializer));

                    auto assignment = std::make_unique<AssignmentStatement>(std::move(lhs_vec), std::move(rhs_vec));
                    assignment->accept(*this);
                }
            } else {
                // This is a top-level, non-VALOF global declaration.
                handle_global_variable_declaration(name, initializer);
            }
        }
    }

    debug_print("Finished visiting LetDeclaration node.");
}

// --- Private Helper Methods ---

bool NewCodeGenerator::is_function_like_declaration(const ExprPtr& initializer) const {
    return initializer && initializer->getType() == ASTNode::NodeType::ValofExpr;
}

bool NewCodeGenerator::is_valof_block_variable(const std::string& name) const {
    // A simple check to see if we are in a VALOF block context.
    // This assumes current_frame_manager_ is null in global scope AND VALOF blocks.
    // A more robust solution might involve a separate flag.
    return !current_frame_manager_ && current_scope_symbols_.count(name) == 0;
}

void NewCodeGenerator::handle_function_like_declaration(const std::string& name, const ExprPtr& initializer) {
    ValofExpression* valof_expr = static_cast<ValofExpression*>(initializer.get());
    debug_print("Detected function-like LetDeclaration: " + name);
    // The following assumes a `generate_function_like_code` exists and takes these arguments
    // as it's not provided in the original snippet.
    generate_function_like_code(name, {}, *valof_expr->body, true);
}

void NewCodeGenerator::handle_local_variable_declaration(const std::string& name, bool is_float) {
    debug_print("Processing local LetDeclaration for already-registered variable: " + name);
    // The variable is already registered by the pre-scan.
    // Update the variable type if it's a float declaration
    if (is_float) {
        debug_print("Variable '" + name + "' declared as float type via FLET");
        if (current_frame_manager_) {
            current_frame_manager_->mark_variable_as_float(name);
        }
    }
    // This function's only job now is to handle the initializer if one exists.
}

void NewCodeGenerator::handle_valof_block_variable(const std::string& name) {
    debug_print("Registering variable '" + name + "' in VALOF block.");
    if (!current_scope_symbols_.count(name)) {
        current_scope_symbols_[name] = -1; // Mark as local in VALOF
        debug_print("Marked '" + name + "' as local in VALOF block.");
    }
}

// This function is now deprecated in favor of using AssignmentStatement
// It is kept for backward compatibility with any existing callers
void NewCodeGenerator::initialize_variable(const std::string& name, const ExprPtr& initializer, bool is_float_declaration) {
    debug_print("WARNING: Using deprecated initialize_variable - should use AssignmentStatement generation instead.");
    
    generate_expression_code(*initializer);
    std::string source_reg = expression_result_reg_;

    int offset = current_frame_manager_->get_offset(name);
    bool source_is_fp = register_manager_.is_fp_register(source_reg);
    
    // Handle mismatch between variable type and value type
    if (is_float_declaration && !source_is_fp) {
        // Convert integer to float if needed
        debug_print("Converting integer to float for FLET variable '" + name + "'");
        std::string temp_fp_reg = register_manager_.acquire_fp_scratch_reg();
        emit(Encoder::create_scvtf_reg(temp_fp_reg, source_reg));
        register_manager_.release_register(source_reg);
        source_reg = temp_fp_reg;
        source_is_fp = true;
    } else if (!is_float_declaration && source_is_fp) {
        // Normally we'd convert float to int here, but we'll keep the float
        // and mark the variable as having float type
        debug_print("Variable '" + name + "' initialized with float value, marking as float type");
        if (current_frame_manager_) {
            current_frame_manager_->mark_variable_as_float(name);
        }
    }

    // Use the smart store_variable_register function which respects the allocator's decisions
    store_variable_register(name, source_reg);
    register_manager_.release_register(source_reg);
    debug_print("Initialized local variable '" + name + "' from expression.");
}

void NewCodeGenerator::handle_global_variable_declaration(const std::string& name, const ExprPtr& initializer) {
    if (!initializer) {
        data_generator_.add_global_variable(name, nullptr);
        debug_print("Added global variable '" + name + "' with default zero initialization.");
        return;
    }
    debug_print("Processing global LetDeclaration: " + name);
    // The variable will be zero-initialized by default.
    data_generator_.add_global_variable(name, std::unique_ptr<Expression>(dynamic_cast<Expression*>(initializer->clone().release())));

    // If there is an initializer, we need to handle it.
    if (initializer) {
        if (NumberLiteral* num_lit = dynamic_cast<NumberLiteral*>(initializer.get())) {
            // Overwrite the placeholder with the actual value.
            data_generator_.add_global_variable(name, std::unique_ptr<Expression>(dynamic_cast<Expression*>(initializer->clone().release())));
            debug_print("Added global numeric variable '" + name + "' for deferred data generation.");
        } else if (StringLiteral* str_lit = dynamic_cast<StringLiteral*>(initializer.get())) {
            // String literals require a more complex setup where the global variable's
            // memory location holds the address of the string data.
            // DataGenerator will handle creating the string data in .rodata.
            data_generator_.add_string_literal(str_lit->value);
            debug_print("Added global string literal '" + name + "' for deferred data generation.");
        } else {
            // For other expression types, we assume they will be initialized at runtime.
            // The default zero initialization is sufficient for now.
            debug_print("Global LetDeclaration: Unsupported initializer type for variable '" + name + "'. Using default zero.");
        }
    }
    debug_print("Added global variable '" + name + "' for deferred data generation.");
}
