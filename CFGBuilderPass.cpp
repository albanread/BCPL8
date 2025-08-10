#include "CFGBuilderPass.h"

#include "BasicBlock.h"
#include <iostream>

CFGBuilderPass::CFGBuilderPass(bool trace_enabled)
    : current_cfg(nullptr),
      current_basic_block(nullptr),
      block_id_counter(0),
      trace_enabled_(trace_enabled) {
    // Initialize stacks for control flow targets
    break_targets.clear();
    loop_targets.clear();
    endcase_targets.clear();
}

void CFGBuilderPass::debug_print(const std::string& message) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] " << message << std::endl;
    }
}

// getName() is defined in the header, no need to redefine here.
// get_cfgs() is defined in the header, no need to redefine here.

// Implementation of visit(GlobalVariableDeclaration& node)
void CFGBuilderPass::visit(GlobalVariableDeclaration& node) {
    // Handle global variable declarations
    for (const auto& name : node.names) {
        if (trace_enabled_) {
            std::cout << "Global variable declared: " << name << std::endl;
        }
    }
}

// Helper to create a new basic block and add it to the current CFG
BasicBlock* CFGBuilderPass::create_new_basic_block(const std::string& id_prefix) {
    if (!current_cfg) {
        if (trace_enabled_) {
            std::cerr << "[CFGBuilderPass] ERROR: Cannot create basic block without an active CFG!" << std::endl;
        }
        throw std::runtime_error("Error: Cannot create basic block without an active CFG.");
    }
    BasicBlock* new_bb = nullptr;
    try {
        new_bb = current_cfg->create_block(id_prefix);
        if (!new_bb) {
            std::cerr << "[CFGBuilderPass] ERROR: create_block returned nullptr!" << std::endl;
        } else {
            if (trace_enabled_) {
                std::cout << "[CFGBuilderPass] Created new basic block: " << new_bb->id << std::endl;
            }
        }
    } catch (const std::exception& ex) {
        if (trace_enabled_) {
            std::cerr << "[CFGBuilderPass] Exception in create_new_basic_block: " << ex.what() << std::endl;
        }
        throw;
    } catch (...) {
        if (trace_enabled_) {
            std::cerr << "[CFGBuilderPass] Unknown exception in create_new_basic_block" << std::endl;
        }
        throw;
    }
    return new_bb;
}

void CFGBuilderPass::visit(FloatValofExpression& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] Visiting FloatValofExpression node." << std::endl;
    }

    // Create a new basic block for the VALOF body
    BasicBlock* valof_body_block = create_new_basic_block("VALOF_BODY");
    current_cfg->add_edge(current_basic_block, valof_body_block);
    current_basic_block = valof_body_block;

    // Visit the body of the VALOF expression
    if (node.body) {
        node.body->accept(*this);
    }

    // Create a new basic block for the continuation after VALOF
    BasicBlock* continuation_block = create_new_basic_block("VALOF_CONT");
    current_cfg->add_edge(current_basic_block, continuation_block);
    current_basic_block = continuation_block;

    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] Finished visiting FloatValofExpression node." << std::endl;
    }
}

// Helper to end the current basic block and start a new one, adding a fall-through edge
void CFGBuilderPass::end_current_block_and_start_new() {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] end_current_block_and_start_new called." << std::endl;
    }
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        BasicBlock* next_bb = nullptr;
        try {
            next_bb = create_new_basic_block();
            if (!next_bb) {
                std::cerr << "[CFGBuilderPass] ERROR: next_bb is nullptr in end_current_block_and_start_new!" << std::endl;
            } else {
                if (trace_enabled_) {
                    std::cout << "[CFGBuilderPass] Adding edge from " << current_basic_block->id << " to " << next_bb->id << std::endl;
                }
                current_cfg->add_edge(current_basic_block, next_bb);
                current_basic_block = next_bb;
            }
        } catch (const std::exception& ex) {
            if (trace_enabled_) {
                std::cerr << "[CFGBuilderPass] Exception in end_current_block_and_start_new: " << ex.what() << std::endl;
            }
            throw;
        } catch (...) {
            if (trace_enabled_) {
                std::cerr << "[CFGBuilderPass] Unknown exception in end_current_block_and_start_new" << std::endl;
            }
            throw;
        }
    } else if (!current_basic_block) {
        // If current_basic_block is nullptr (e.g., after a GOTO or RETURN),
        // we just need to create a new block for the next statement.
        if (trace_enabled_) {
            std::cout << "[CFGBuilderPass] current_basic_block is nullptr, creating new block." << std::endl;
        }
        current_basic_block = create_new_basic_block();
    }
    // If current_basic_block ends with control flow, it means it explicitly transfers control,
    // so no implicit fall-through to a new block is needed from here.
}

void CFGBuilderPass::build(Program& program) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] build() called." << std::endl;
    }
    // Reset state for a new program build
    function_cfgs.clear();
    current_cfg = nullptr;
    current_basic_block = nullptr;
    label_targets.clear();
    block_id_counter = 0;
    break_targets.clear();
    loop_targets.clear();
    endcase_targets.clear();

    // Start the traversal from the Program node
    try {
        if (trace_enabled_) {
            std::cout << "[CFGBuilderPass] About to accept(Program)" << std::endl;
        }
        program.accept(*this);
        if (trace_enabled_) {
            std::cout << "[CFGBuilderPass] Finished accept(Program)" << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[CFGBuilderPass] Exception during build: " << ex.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "[CFGBuilderPass] Unknown exception during build" << std::endl;
        throw;
    }
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] build() complete." << std::endl;
    }
}

// --- ASTVisitor overrides (implementing only the necessary ones for CFG building) ---

void CFGBuilderPass::visit(Program& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(Program) called." << std::endl;
    }
    // Iterate through declarations and statements to find functions/routines
    int decl_idx = 0;
    for (const auto& decl : node.declarations) {
        if (trace_enabled_) {
            std::cout << "[CFGBuilderPass] Processing declaration #" << decl_idx << std::endl;
        }
        if (!decl) {
            std::cerr << "[CFGBuilderPass] WARNING: Null declaration at index " << decl_idx << std::endl;
            ++decl_idx;
            continue;
        }
        try {
            if (decl->getType() == ASTNode::NodeType::FunctionDecl) {
                if (trace_enabled_) std::cout << "[CFGBuilderPass] Found FunctionDecl at index " << decl_idx << std::endl;
                visit(static_cast<FunctionDeclaration&>(*decl));
            } else if (decl->getType() == ASTNode::NodeType::RoutineDecl) {
                if (trace_enabled_) std::cout << "[CFGBuilderPass] Found RoutineDecl at index " << decl_idx << std::endl;
                visit(static_cast<RoutineDeclaration&>(*decl));
            }
        } catch (const std::exception& ex) {
            std::cerr << "[CFGBuilderPass] Exception in visit(Program) for declaration #" << decl_idx << ": " << ex.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "[CFGBuilderPass] Unknown exception in visit(Program) for declaration #" << decl_idx << std::endl;
            throw;
        }
        ++decl_idx;
        // Other declarations (LET, MANIFEST, STATIC, GLOBAL, GET) don't create CFGs themselves
        // but their initializers/values might contain expressions that need to be visited
        // if they are part of a function/routine body.
        // For now, we focus on function/routine bodies.
    }

    // Handle top-level statements if they are considered part of a 'main' CFG
    // For simplicity, we'll assume functions/routines are the primary units for CFGs.
    // If top-level statements need a CFG, a 'main' function CFG would be created here.
    // For now, we'll skip them.
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(Program) complete." << std::endl;
    }
}

void CFGBuilderPass::visit(FunctionDeclaration& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(FunctionDeclaration) for function: " << node.name << std::endl;
    }
    // Start a new CFG for this function
    current_cfg = new ControlFlowGraph(node.name); // Create new CFG
    function_cfgs[node.name].reset(current_cfg); // Transfer ownership

    block_id_counter = 0; // Reset block ID counter for each new function
    label_targets.clear(); // Clear labels for new function scope
    unresolved_gotos_.clear(); // Clear pending GOTOs for the new function

    // Create the entry block for the function
    current_basic_block = create_new_basic_block("Entry_");
    if (current_basic_block) {
        current_basic_block->is_entry = true;
        current_cfg->entry_block = current_basic_block;
    } else {
        std::cerr << "[CFGBuilderPass] ERROR: Failed to create entry block for function: " << node.name << std::endl;
        return;
    }

    // Visit the function body
    if (node.body) {
        try {
            node.body->accept(*this);
        } catch (const std::exception& ex) {
            std::cerr << "[CFGBuilderPass] Exception in visit(FunctionDeclaration) body: " << ex.what() << std::endl;
            throw;
        } catch (...) {
            std::cerr << "[CFGBuilderPass] Unknown exception in visit(FunctionDeclaration) body" << std::endl;
            throw;
        }
    } else {
        std::cerr << "[CFGBuilderPass] WARNING: Function " << node.name << " has no body." << std::endl;
    }

    // Ensure the last block has an edge to the exit block if it doesn't end with control flow
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        if (!current_cfg->exit_block) {
            current_cfg->exit_block = create_new_basic_block("Exit_");
            if (current_cfg->exit_block) {
                current_cfg->exit_block->is_exit = true;
            } else {
                std::cerr << "[CFGBuilderPass] ERROR: Failed to create exit block for function: " << node.name << std::endl;
            }
        }
        if (current_cfg->exit_block) {
            current_cfg->add_edge(current_basic_block, current_cfg->exit_block);
        }
    }

    // After traversing the entire function body, resolve all GOTO statements.
    resolve_gotos();
    
    // Reset current CFG and basic block pointers
    current_cfg = nullptr;
    current_basic_block = nullptr;
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(FunctionDeclaration) complete for function: " << node.name << std::endl;
    }
}

void CFGBuilderPass::visit(RoutineDeclaration& node) {
    // Similar to FunctionDeclaration, but body is a statement
    current_cfg = new ControlFlowGraph(node.name);
    function_cfgs[node.name].reset(current_cfg);

    block_id_counter = 0;
    label_targets.clear();
    unresolved_gotos_.clear(); // Clear pending GOTOs for the new routine

    current_basic_block = create_new_basic_block("Entry_");
    current_basic_block->is_entry = true;
    current_cfg->entry_block = current_basic_block;

    if (node.body) {
        node.body->accept(*this);
    }

    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        if (!current_cfg->exit_block) {
            current_cfg->exit_block = create_new_basic_block("Exit_");
            current_cfg->exit_block->is_exit = true;
        }
        current_cfg->add_edge(current_basic_block, current_cfg->exit_block);
    }

    // After traversing the entire routine body, resolve all GOTO statements.
    resolve_gotos();

    current_cfg = nullptr;
    current_basic_block = nullptr;
}

// This helper function might be needed at the top of the file if it's not already accessible.
// It should be defined in AST_Cloner.cpp or AST.cpp.


template <typename T>
std::unique_ptr<T> clone_unique_ptr(const std::unique_ptr<T>& original_ptr) {
    if (original_ptr) {
        return std::unique_ptr<T>(static_cast<T*>(original_ptr->clone().release()));
    }
    return nullptr;
}

void CFGBuilderPass::visit(LetDeclaration& node) {
    if (!current_basic_block) {
        end_current_block_and_start_new();
    }

    // A LET declaration can have multiple assignments, e.g., LET A, B = 1, 2
    for (size_t i = 0; i < node.names.size(); ++i) {
        if (i < node.initializers.size() && node.initializers[i]) {
            // First, traverse the initializer expression itself.
            node.initializers[i]->accept(*this);

            // Create an AssignmentStatement to represent the LET's action.
            auto lhs = std::make_unique<VariableAccess>(node.names[i]);

            std::vector<ExprPtr> lhs_vec;
            lhs_vec.push_back(std::move(lhs));
            std::vector<ExprPtr> rhs_vec;
            rhs_vec.push_back(clone_unique_ptr<Expression>(node.initializers[i]));
            auto assignment = std::make_unique<AssignmentStatement>(
                std::move(lhs_vec),
                std::move(rhs_vec)
            );

            // *** This is the crucial step that was missing ***
            // Add the new assignment to the current basic block.
            current_basic_block->add_statement(std::move(assignment));
        }
    }
}
void CFGBuilderPass::visit(ManifestDeclaration& node) {
    // Manifests are compile-time constants, no runtime code or CFG impact.
}
void CFGBuilderPass::visit(StaticDeclaration& node) {
    // Static initializers are expressions, visit them.
    if (node.initializer) node.initializer->accept(*this);
}
void CFGBuilderPass::visit(GlobalDeclaration& node) {
    // Globals are compile-time offsets, no runtime code or CFG impact.
}

// Implementation for GlobalVariableDeclaration is already defined above


// Expressions - these don't create new basic blocks, just add to current
void CFGBuilderPass::visit(NumberLiteral& node) { /* No CFG specific action */ }
void CFGBuilderPass::visit(StringLiteral& node) { /* No CFG specific action */ }
void CFGBuilderPass::visit(CharLiteral& node) { /* No CFG specific action */ }
void CFGBuilderPass::visit(BooleanLiteral& node) { /* No CFG specific action */ }
void CFGBuilderPass::visit(VariableAccess& node) { /* No CFG specific action */ }
void CFGBuilderPass::visit(BinaryOp& node) {
    node.left->accept(*this);
    node.right->accept(*this);
}
void CFGBuilderPass::visit(UnaryOp& node) {
    node.operand->accept(*this);
}
void CFGBuilderPass::visit(VectorAccess& node) {
    node.vector_expr->accept(*this);
    node.index_expr->accept(*this);
}
void CFGBuilderPass::visit(CharIndirection& node) {
    node.string_expr->accept(*this);
    node.index_expr->accept(*this);
}
void CFGBuilderPass::visit(FloatVectorIndirection& node) {
    node.vector_expr->accept(*this);
    node.index_expr->accept(*this);
}
void CFGBuilderPass::visit(FunctionCall& node) {
    node.function_expr->accept(*this);
    for (const auto& arg : node.arguments) {
        arg->accept(*this);
    }
}
void CFGBuilderPass::visit(ConditionalExpression& node) {
    node.condition->accept(*this);
    node.true_expr->accept(*this);
    node.false_expr->accept(*this);
}
void CFGBuilderPass::visit(ValofExpression& node) {
    // VALOF is an expression but contains a statement body. Special handling needed.

    // End the current block and start a new one for the VALOF body.
    end_current_block_and_start_new();

    // Visit the body of the VALOF expression.
    if (node.body) {
        node.body->accept(*this);
    }

    // End the VALOF body block and start a new one for subsequent statements.
    end_current_block_and_start_new();
}



// Statements - these might create new basic blocks or add to current
void CFGBuilderPass::visit(AssignmentStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    // Recursively visit children expressions (LHS and RHS) - no need to clone here, just traverse
    for (const auto& lhs_expr : node.lhs) { lhs_expr->accept(*this); }
    for (const auto& rhs_expr : node.rhs) { rhs_expr->accept(*this); }
}

void CFGBuilderPass::visit(RoutineCallStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    // Visit children expressions
    node.routine_expr->accept(*this);
    for (const auto& arg : node.arguments) { arg->accept(*this); }
}

void CFGBuilderPass::visit(IfStatement& node) {
    // 1. Process the condition expression.
    node.condition->accept(*this);

    // 2. Add the IfStatement itself to the current block. This makes it the conditional terminator.
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));

    // --- FIX: Do NOT end the current block here. The current block IS the condition block. ---
    BasicBlock* condition_block = current_basic_block;

    // 3. Create the destination blocks for the branches.
    BasicBlock* then_block = create_new_basic_block("Then_");
    BasicBlock* join_block = create_new_basic_block("Join_");

    // 4. Add edges from the condition block to the two possible paths.
    current_cfg->add_edge(condition_block, then_block); // True path
    current_cfg->add_edge(condition_block, join_block); // False path

    // 5. Visit the 'then' branch, starting in the 'then_block'.
    current_basic_block = then_block;
    node.then_branch->accept(*this);
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, join_block);
    }

    // 6. Set the current block to the join block for subsequent statements.
    current_basic_block = join_block;
}

void CFGBuilderPass::visit(UnlessStatement& node) {
    // 1. The current block contains the code leading up to the UNLESS.
    node.condition->accept(*this);

    // 2. Add the UnlessStatement itself to the current block as the conditional terminator.
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));

    // The current block IS the condition block.
    BasicBlock* condition_block = current_basic_block;

    // 3. Create the destination blocks for the branches.
    BasicBlock* then_block = create_new_basic_block("Then_");
    BasicBlock* join_block = create_new_basic_block("Join_");

    // 4. Add edges from the condition block to the two possible paths.
    current_cfg->add_edge(condition_block, then_block); // False condition (UNLESS is true)
    current_cfg->add_edge(condition_block, join_block); // True condition (UNLESS is false)

    // 5. Visit the 'then' branch, starting in its own new block.
    current_basic_block = then_block;
    node.then_branch->accept(*this);
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, join_block);
    }

    // 6. Set the current block to the join block for subsequent statements.
    current_basic_block = join_block;
}

void CFGBuilderPass::visit(TestStatement& node) {
    node.condition->accept(*this);
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    
    // The current block IS the condition block.
    BasicBlock* condition_block = current_basic_block;

    BasicBlock* then_block = create_new_basic_block("Then_");
    BasicBlock* else_block = create_new_basic_block("Else_");
    BasicBlock* join_block = create_new_basic_block("Join_");

    current_cfg->add_edge(condition_block, then_block); // True path
    current_cfg->add_edge(condition_block, else_block); // False path

    // Visit then branch
    current_basic_block = then_block;
    node.then_branch->accept(*this);
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, join_block);
    }

    // Visit else branch (if it exists)
    if (node.else_branch) {
        current_basic_block = else_block;
        node.else_branch->accept(*this);
        if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
            current_cfg->add_edge(current_basic_block, join_block);
        }
    } else {
        // If there's no else branch, the false path from the condition goes directly to join.
        current_cfg->add_edge(else_block, join_block);
    }

    current_basic_block = join_block;
}

void CFGBuilderPass::visit(WhileStatement& node) {
    // 1. The current block is the loop header.
    BasicBlock* loop_header = current_basic_block;
    loop_targets.push_back(loop_header); // For LOOP statement

    // 2. Process condition (in loop header)
    node.condition->accept(*this);
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); // Add the WhileStatement itself

    // 3. Create loop body block and exit block
    BasicBlock* loop_body = create_new_basic_block("WhileBody_");
    BasicBlock* loop_exit = create_new_basic_block("WhileExit_");
    break_targets.push_back(loop_exit); // For BREAK statement

    // 4. Add edges from header based on condition
    current_cfg->add_edge(loop_header, loop_body); // True condition
    current_cfg->add_edge(loop_header, loop_exit); // False condition

    // 5. Visit loop body
    current_basic_block = loop_body;
    node.body->accept(*this);

    // 6. Add edge from end of body back to loop header (for iteration)
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, loop_header);
    }

    // 7. Set current block to loop exit
    current_basic_block = loop_exit;

    loop_targets.pop_back();
    break_targets.pop_back();
}

void CFGBuilderPass::visit(UntilStatement& node) {
    // --- FIX: Do NOT end the current block before the loop header. The current block IS the loop header. ---
    BasicBlock* loop_header = current_basic_block;
    loop_targets.push_back(loop_header); // For LOOP statement

    // Process condition (in loop header)
    node.condition->accept(*this);
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); // Add the UntilStatement itself

    // Create loop body block and exit block
    BasicBlock* loop_body = create_new_basic_block("UntilBody_");
    BasicBlock* loop_exit = create_new_basic_block("UntilExit_");
    break_targets.push_back(loop_exit); // For BREAK statement

    // Add edges from header based on condition
    current_cfg->add_edge(loop_header, loop_body); // False condition (UNTIL is true)
    current_cfg->add_edge(loop_header, loop_exit); // True condition (UNTIL is false)

    // Visit loop body
    current_basic_block = loop_body;
    node.body->accept(*this);

    // Add edge from end of body back to loop header (for iteration)
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, loop_header);
    }
    // Set current block to loop exit
    current_basic_block = loop_exit;

    loop_targets.pop_back();
    break_targets.pop_back();
}

void CFGBuilderPass::visit(RepeatStatement& node) {
    // 1. End the current block and create a new, dedicated block for the loop body.
    end_current_block_and_start_new();
    
    // This is the actual entry point for the loop body - where LOOP should jump to
    BasicBlock* loop_entry = current_basic_block;
    
    // Create an exit block for the loop - this is where BREAK statements will jump to
    BasicBlock* loop_exit = create_new_basic_block("RepeatExit_");
    
    // Push the loop entry and exit targets for LOOP and BREAK statements
    loop_targets.push_back(loop_entry);
    break_targets.push_back(loop_exit);
    
    // Create a block for the condition check that will be used after the loop body
    BasicBlock* condition_block = create_new_basic_block("RepeatCondition_");

    // 2. Visit loop body
    node.body->accept(*this);
    
    // If we encountered a LOOP statement, current_basic_block will be nullptr
    // In that case, we can just continue with the condition block
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        // Only add an edge if the current block doesn't already end with control flow
        // (like a LOOP statement would create)
        debug_print("Adding edge from loop body to condition block");
        current_cfg->add_edge(current_basic_block, condition_block);
    } else {
        debug_print("Current block either null or ends with control flow - not adding edge to condition");
    }
    
    // Set the current block to the condition block for the loop condition evaluation
    current_basic_block = condition_block;

    // 3. Handle different REPEAT types
    if (node.loop_type == RepeatStatement::LoopType::Repeat) {
        // Simple REPEAT: unconditional loop back to entry
        if (!current_basic_block->ends_with_control_flow()) {
            current_cfg->add_edge(current_basic_block, loop_entry);
        }
    } else if (node.loop_type == RepeatStatement::LoopType::RepeatWhile) {
        // REPEAT <body> WHILE <condition>
        // Condition check happens after body execution
        node.condition->accept(*this);
        current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); // Add the RepeatStatement itself

        current_cfg->add_edge(current_basic_block, loop_entry); // True condition: loop back
        current_cfg->add_edge(current_basic_block, loop_exit);  // False condition: exit
    } else if (node.loop_type == RepeatStatement::LoopType::RepeatUntil) {
        // REPEAT <body> UNTIL <condition>
        // Condition check happens after body execution
        node.condition->accept(*this);
        current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); // Add the RepeatStatement itself

        current_cfg->add_edge(current_basic_block, loop_exit);  // True condition: exit
        current_cfg->add_edge(current_basic_block, loop_entry); // False condition: loop back
    }

    // 4. Set current block to loop exit
    current_basic_block = loop_exit;

    // Now that we're done with the loop, pop the targets
    // Before we finish processing the repeat statement, we need to ensure
    // any code that comes after a LOOP statement is reachable. Fix any null
    // blocks that were set by the LOOP statement handling.
    if (!current_basic_block) {
        current_basic_block = create_new_basic_block();
        // Note: We don't add an edge from the LOOP block to this block, because
        // it should be unreachable. The LOOP block jumps directly to the loop start.
    }
    
    loop_targets.pop_back();
    break_targets.pop_back();
}

void CFGBuilderPass::visit(ForStatement& node) {
    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(ForStatement) entered." << std::endl;

    // 1. Initialization happens in the current block, before the loop header.
    if (trace_enabled_) std::cout << "[CFGBuilderPass] About to process start_expr for ForStatement" << std::endl;
    node.start_expr->accept(*this);
    if (trace_enabled_) std::cout << "[CFGBuilderPass] Creating initialization assignment: " << node.unique_loop_variable_name << " := start_expr" << std::endl;
    auto lhs_init = std::make_unique<VariableAccess>(node.unique_loop_variable_name);
    std::vector<ExprPtr> lhs_vec;
    lhs_vec.push_back(std::move(lhs_init));
    std::vector<ExprPtr> rhs_vec;
    rhs_vec.push_back(clone_unique_ptr(node.start_expr));
    current_basic_block->add_statement(std::make_unique<AssignmentStatement>(std::move(lhs_vec), std::move(rhs_vec)));
    if (trace_enabled_) std::cout << "[CFGBuilderPass] Added initialization assignment to block: " << current_basic_block->id << std::endl;

    // 2. Create the loop header block, which contains the condition.
    // Fix: Don't break the connection - manually create and connect the loop header
    BasicBlock* init_block = current_basic_block;
    BasicBlock* loop_header = create_new_basic_block("BB_");
    current_cfg->add_edge(init_block, loop_header);
    current_basic_block = loop_header;  // Set current block to loop header for processing
    loop_targets.push_back(loop_header);

    // 3. Create body and exit blocks.
    BasicBlock* loop_body = create_new_basic_block("ForBody_");
    BasicBlock* loop_exit = create_new_basic_block("ForExit_");
    break_targets.push_back(loop_exit);

    // 4. Add the ForStmt node to the header block to represent the condition check.
    if (trace_enabled_) std::cout << "[CFGBuilderPass] Adding ForStatement clone to loop header block: " << loop_header->id << std::endl;
    loop_header->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    current_cfg->add_edge(loop_header, loop_body); // True -> Enter body
    current_cfg->add_edge(loop_header, loop_exit); // False -> Exit loop

    // 5. Visit the loop body.
    current_basic_block = loop_body;
    node.body->accept(*this);

    // 6. At the end of the body, add the increment step.
    if (current_basic_block) {
        // Create an increment block for the step expression
        BasicBlock* increment_block;
        
        if (current_basic_block->ends_with_control_flow()) {
            // If the body ends with control flow, we need a new block for the increment
            increment_block = create_new_basic_block("ForIncr_");
            if (trace_enabled_) std::cout << "[CFGBuilderPass] Created new increment block: " << increment_block->id << std::endl;
            current_cfg->add_edge(current_basic_block, increment_block);
            current_basic_block = increment_block;
        } else {
            // Use the current block for increment
            increment_block = current_basic_block;
        }
        
        // Create the increment assignment: loop_var = loop_var + step
        auto lhs_incr = std::make_unique<VariableAccess>(node.unique_loop_variable_name);
        std::vector<ExprPtr> lhs_incr_vec;
        lhs_incr_vec.push_back(std::move(lhs_incr));
        std::vector<ExprPtr> rhs_incr_vec;
        
        // If no step expression provided, use 1 as the default step
        if (node.step_expr) {
            rhs_incr_vec.push_back(std::make_unique<BinaryOp>(
                BinaryOp::Operator::Add,
                std::make_unique<VariableAccess>(node.unique_loop_variable_name),
                clone_unique_ptr(node.step_expr)
            ));
        } else {
            rhs_incr_vec.push_back(std::make_unique<BinaryOp>(
                BinaryOp::Operator::Add,
                std::make_unique<VariableAccess>(node.unique_loop_variable_name),
                std::make_unique<NumberLiteral>(static_cast<int64_t>(1))
            ));
        }
        
        increment_block->add_statement(std::make_unique<AssignmentStatement>(std::move(lhs_incr_vec), std::move(rhs_incr_vec)));
        if (trace_enabled_) std::cout << "[CFGBuilderPass] Added increment assignment to block: " << increment_block->id << std::endl;
        
        // 7. Add the edge back to the loop header.
        current_cfg->add_edge(increment_block, loop_header);
    } else {
        // Error case: no current block to add increment to
        if (trace_enabled_) std::cout << "[CFGBuilderPass] Warning: No valid block to add for loop increment" << std::endl;
        // Still ensure we have the loop back edge
        current_cfg->add_edge(loop_body, loop_header);
    }

    // 8. Set the current block to the exit block.
    current_basic_block = loop_exit;
    loop_targets.pop_back();
    break_targets.pop_back();
    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(ForStatement) exiting." << std::endl;
}


void CFGBuilderPass::visit(SwitchonStatement& node) {
    // 1. Process the switch expression
    node.expression->accept(*this);

    // 2. End current block and start a new one for the switch expression
    end_current_block_and_start_new();
    BasicBlock* switch_expr_block = current_basic_block;

    // 3. Skip creating a basic block for the INTO block

    // 4. Skip INTO block handling as it is no longer part of SwitchonStatement

    // 5. After the INTO block, create a new block for case dispatch
    end_current_block_and_start_new();
    BasicBlock* case_dispatch_block = current_basic_block;

    // 6. Create the end block for the switch
    BasicBlock* end_switch_block = create_new_basic_block("SwitchEnd_");
    endcase_targets.push_back(end_switch_block); // For ENDCASE statement

    // 7. Add the SwitchonStatement itself to the case dispatch block
    current_basic_block = case_dispatch_block;
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));

    // 8. Handle cases
    for (const auto& case_stmt : node.cases) {
        BasicBlock* case_block = create_new_basic_block("Case_");
        current_cfg->add_edge(case_dispatch_block, case_block); // Edge from case dispatch to case

        current_basic_block = case_block;
        case_stmt->command->accept(*this);
        if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
            current_cfg->add_edge(current_basic_block, end_switch_block);
        }
    }

    // 9. Handle default case
    if (node.default_case) {
        BasicBlock* default_block = create_new_basic_block("Default_");
        current_cfg->add_edge(case_dispatch_block, default_block); // Edge from case dispatch to default

        current_basic_block = default_block;
        node.default_case->command->accept(*this);
        if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
            current_cfg->add_edge(current_basic_block, end_switch_block);
        }
    } else {
        // If no default, fall through to end_switch_block if no case matches
        current_cfg->add_edge(case_dispatch_block, end_switch_block);
    }

    current_basic_block = end_switch_block;
    endcase_targets.pop_back();
}

void CFGBuilderPass::visit(CaseStatement& node) {
    // Case statements are handled by SwitchonStatement visitor
    // Their constant_expr and command are visited there.
}

void CFGBuilderPass::visit(LabelDeclaration& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] WARNING: LabelDeclaration is deprecated. Use LabelTargetStatement instead." << std::endl;
    }
    // Labels are handled by creating a new basic block that is the target of GOTO statements.
    // The label itself doesn't generate code, but it marks a location.
    if (current_basic_block != nullptr && !current_basic_block->ends_with_control_flow()) {
        end_current_block_and_start_new();
    }
    BasicBlock* label_block = create_new_basic_block("Label_" + node.name);
    label_targets[node.name] = label_block;
    current_basic_block = label_block;
    node.command->accept(*this);
}

// ADD the new visit method for LabelTargetStatement:
void CFGBuilderPass::visit(LabelTargetStatement& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] Visiting LabelTargetStatement: " << node.labelName << std::endl;
    }
    
    // Get the block that comes before the label.
    BasicBlock* predecessor_block = current_basic_block;

    // Create the new block that starts at this label.
    BasicBlock* label_block = create_new_basic_block("Label_" + node.labelName);
    label_targets[node.labelName] = label_block;

    // If the predecessor block exists and doesn't end in a GOTO/RETURN,
    // create a fall-through edge to this new label block.
    if (predecessor_block && !predecessor_block->ends_with_control_flow()) {
        current_cfg->add_edge(predecessor_block, label_block);
    }

    // This new block is now the current one.
    current_basic_block = label_block;
    
    // Add the LabelTargetStatement itself to the block's statement list.
    // This can be useful for later stages.
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
}

void CFGBuilderPass::visit(DefaultStatement& node) {
    // Default statements are handled by SwitchonStatement visitor
    // Their command is visited there.
}

void CFGBuilderPass::visit(GotoStatement& node) {
    // Add GOTO statement to current block
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));

    // Defer the resolution of this GOTO by storing it.
    unresolved_gotos_.push_back({&node, current_basic_block});
    
    // The rest of the function remains the same.
    current_basic_block = nullptr;
}

/**
 * @brief Resolves all pending GOTO statements by creating edges in the CFG.
 * This should be called after a function/routine has been fully traversed.
 */
void CFGBuilderPass::resolve_gotos() {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] Resolving " << unresolved_gotos_.size() << " GOTO statements..." << std::endl;
    }
    for (const auto& pair : unresolved_gotos_) {
        GotoStatement* goto_stmt = pair.first;
        BasicBlock* from_block = pair.second;

        if (auto* var_access = dynamic_cast<VariableAccess*>(goto_stmt->label_expr.get())) {
            const std::string& label_name = var_access->name;
            auto it = label_targets.find(label_name);
            if (it != label_targets.end()) {
                BasicBlock* to_block = it->second;
                current_cfg->add_edge(from_block, to_block);
                if (trace_enabled_) {
                    std::cout << "[CFGBuilderPass]   Added GOTO edge from " << from_block->id << " -> " << to_block->id << " (Label: " << label_name << ")" << std::endl;
                }
            } else {
                std::cerr << "[CFGBuilderPass] ERROR: GOTO target label '" << label_name << "' not found!" << std::endl;
            }
        } else {
            // This is a computed GOTO (e.g., GOTO T!I). We cannot resolve its target at compile time.
            // The code generator will have to handle this with an indirect branch (`BR`).
            // For now, we'll just note it.
             if (trace_enabled_) {
                std::cout << "[CFGBuilderPass]   Skipping edge creation for computed GOTO in block " << from_block->id << std::endl;
            }
        }
    }
}

void CFGBuilderPass::visit(ReturnStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    if (!current_cfg->exit_block) {
        current_cfg->exit_block = create_new_basic_block("Exit_");
        current_cfg->exit_block->is_exit = true;
    }
    current_cfg->add_edge(current_basic_block, current_cfg->exit_block);
    current_basic_block = nullptr; // No fall-through
}

void CFGBuilderPass::visit(FinishStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    // FINISH is like an exit from the program, not just the function.
    // For CFG purposes, it can be treated as an edge to a global exit block or the function's exit block.
    if (!current_cfg->exit_block) {
        current_cfg->exit_block = create_new_basic_block("Exit_");
        current_cfg->exit_block->is_exit = true;
    }
    current_cfg->add_edge(current_basic_block, current_cfg->exit_block);
    current_basic_block = nullptr; // No fall-through
}

void CFGBuilderPass::visit(BreakStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    if (!break_targets.empty()) {
        current_cfg->add_edge(current_basic_block, break_targets.back());
    } else {
        // Error: BREAK outside loop/switch
        std::cerr << "Warning: BREAK statement outside of a loop or switch at line X, col Y.\n";
    }
    current_basic_block = nullptr; // No fall-through
}

void CFGBuilderPass::visit(LoopStatement& node) {
    // Add the LOOP statement to the current block
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    
    if (!loop_targets.empty()) {
        // When we encounter a LOOP statement, we need to jump to the actual loop entry point
        // Get the innermost loop's entry block from loop_targets
        BasicBlock* loop_start = loop_targets.back();
        
        // Add an edge directly from the current block to the loop entry point
        current_cfg->add_edge(current_basic_block, loop_start);
        
        if (trace_enabled_) {
            std::cerr << "DEBUG: LOOP statement - Adding edge from " << 
                      current_basic_block->id << " to loop start " << loop_start->id << 
                      " (total loop targets: " << loop_targets.size() << ")\n";
        }
        debug_print("Adding edge from block containing LOOP to loop start: " + 
                    current_basic_block->id + " -> " + loop_start->id);
                    
        // Since LOOP is like 'continue' in C, we terminate the current block
        // and any code after the LOOP will be in a new block (which will be unreachable)
        end_current_block_and_start_new();
    } else {
        std::cerr << "Warning: LOOP statement outside of a loop context at line X, col Y.\n";
    }
}

void CFGBuilderPass::visit(EndcaseStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    if (!endcase_targets.empty()) {
        current_cfg->add_edge(current_basic_block, endcase_targets.back());
    } else {
        // Error: ENDCASE outside switch
        std::cerr << "Warning: ENDCASE statement outside of a SWITCHON at line X, col Y.\n";
    }
    current_basic_block = nullptr; // No fall-through
}

void CFGBuilderPass::visit(ResultisStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    // RESULTIS is like a return from a VALOF block. Special handling needed.
    // For now, treat as a return from the current function/routine.
    if (!current_cfg->exit_block) {
        current_cfg->exit_block = create_new_basic_block("Exit_");
        current_cfg->exit_block->is_exit = true;
    }
    current_cfg->add_edge(current_basic_block, current_cfg->exit_block);
    current_basic_block = nullptr; // No fall-through
}

void CFGBuilderPass::visit(CompoundStatement& node) {
    for (const auto& stmt : node.statements) {
        if (current_basic_block == nullptr) {
            // This can happen after a GOTO, RETURN, etc. The next statement
            // that isn't a label needs a new block.
            current_basic_block = create_new_basic_block();
        }
        // DO NOT add the statement here. Just dispatch the visit.
        // The specific visitor (e.g., visit(AssignmentStatement&)) will handle adding the statement.
        stmt->accept(*this);
    }
}

void CFGBuilderPass::visit(BlockStatement& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] Visiting BlockStatement." << std::endl;
    }

    // --- FIX: Process declarations first, as they can define labels. ---
    for (const auto& decl : node.declarations) {
        if (current_basic_block == nullptr) {
            // This can happen after a GOTO, RETURN, etc. The next instruction
            // needs a new, reachable block.
            current_basic_block = create_new_basic_block();
        }
        if (decl) {
            decl->accept(*this);
        }
    }

    // Now, process statements as before.
    for (const auto& stmt : node.statements) {
        if (current_basic_block == nullptr) {
            current_basic_block = create_new_basic_block();
        }
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

void CFGBuilderPass::visit(StringStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    node.size_expr->accept(*this);
}

void CFGBuilderPass::visit(BrkStatement& node) {
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
}

void CFGBuilderPass::visit(FreeStatement& node) {
    // No CFG-specific action for FreeStatement
}

void CFGBuilderPass::visit(ConditionalBranchStatement& node) {
    // No CFG-specific action for ConditionalBranchStatement
}

void CFGBuilderPass::visit(SysCall& node) {
    // No CFG-specific action for SysCall
}

void CFGBuilderPass::visit(VecAllocationExpression& node) {
    // No CFG-specific action for VecAllocationExpression
}

void CFGBuilderPass::visit(StringAllocationExpression& node) {
    // No CFG-specific action for StringAllocationExpression
}

void CFGBuilderPass::visit(TableExpression& node) {
    // No CFG-specific action for TableExpression
}