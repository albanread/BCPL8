#include "CFGBuilderPass.h"
#include "BasicBlock.h"
#include "AST.h"
#include <iostream>
#include <vector>
#include <memory>
#include "analysis/ASTAnalyzer.h"

// Helper function to clone a unique_ptr, assuming it's available from AST_Cloner.cpp
template <typename T>
std::unique_ptr<T> clone_unique_ptr(const std::unique_ptr<T>& original_ptr) {
    if (original_ptr) {
        return std::unique_ptr<T>(static_cast<T*>(original_ptr->clone().release()));
    }
    return nullptr;
}


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
        if (trace_enabled_) {
            std::cout << "[CFGBuilderPass] current_basic_block is nullptr, creating new block." << std::endl;
        }
        current_basic_block = create_new_basic_block();
    }
}

void CFGBuilderPass::build(Program& program) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] build() called." << std::endl;
    }
    function_cfgs.clear();
    current_cfg = nullptr;
    current_basic_block = nullptr;
    label_targets.clear();
    block_id_counter = 0;
    break_targets.clear();
    loop_targets.clear();
    endcase_targets.clear();

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
             if (trace_enabled_) {
                std::cout << "[CFGBuilderPass]   Skipping edge creation for computed GOTO in block " << from_block->id << std::endl;
            }
        }
    }
}


// --- ASTVisitor Overrides ---

void CFGBuilderPass::visit(Program& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(Program) called." << std::endl;
    }
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
    }
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(Program) complete." << std::endl;
    }
}

void CFGBuilderPass::visit(FunctionDeclaration& node) {
    auto& analyzer = ASTAnalyzer::getInstance();
    std::string previous_scope = analyzer.get_current_function_scope();
    analyzer.set_current_function_scope(node.name);

    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(FunctionDeclaration) for function: " << node.name << std::endl;
    }
    current_cfg = new ControlFlowGraph(node.name);
    function_cfgs[node.name].reset(current_cfg);

    block_id_counter = 0;
    label_targets.clear();
    unresolved_gotos_.clear();

    current_basic_block = create_new_basic_block("Entry_");
    if (current_basic_block) {
        current_basic_block->is_entry = true;
        current_cfg->entry_block = current_basic_block;
    } else {
        std::cerr << "[CFGBuilderPass] ERROR: Failed to create entry block for function: " << node.name << std::endl;
        analyzer.set_current_function_scope(previous_scope);
        return;
    }

    if (node.body) {
        try {
            node.body->accept(*this);
        } catch (const std::exception& ex) {
            std::cerr << "[CFGBuilderPass] Exception in visit(FunctionDeclaration) body: " << ex.what() << std::endl;
            analyzer.set_current_function_scope(previous_scope);
            throw;
        } catch (...) {
            std::cerr << "[CFGBuilderPass] Unknown exception in visit(FunctionDeclaration) body" << std::endl;
            analyzer.set_current_function_scope(previous_scope);
            throw;
        }
    } else {
        std::cerr << "[CFGBuilderPass] WARNING: Function " << node.name << " has no body." << std::endl;
    }

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

    resolve_gotos();

    current_cfg = nullptr;
    current_basic_block = nullptr;
    analyzer.set_current_function_scope(previous_scope);
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] visit(FunctionDeclaration) complete for function: " << node.name << std::endl;
    }
}

void CFGBuilderPass::visit(RoutineDeclaration& node) {
    auto& analyzer = ASTAnalyzer::getInstance();
    std::string previous_scope = analyzer.get_current_function_scope();
    analyzer.set_current_function_scope(node.name);

    current_cfg = new ControlFlowGraph(node.name);
    function_cfgs[node.name].reset(current_cfg);

    block_id_counter = 0;
    label_targets.clear();
    unresolved_gotos_.clear();

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

    resolve_gotos();

    current_cfg = nullptr;
    current_basic_block = nullptr;
    analyzer.set_current_function_scope(previous_scope);
}

void CFGBuilderPass::visit(LetDeclaration& node) {
    if (!current_basic_block) {
        end_current_block_and_start_new();
    }

    for (size_t i = 0; i < node.names.size(); ++i) {
        if (i < node.initializers.size() && node.initializers[i]) {
            node.initializers[i]->accept(*this);
            auto lhs = std::make_unique<VariableAccess>(node.names[i]);

            std::vector<ExprPtr> lhs_vec;
            lhs_vec.push_back(std::move(lhs));

            std::vector<ExprPtr> rhs_vec;
            rhs_vec.push_back(clone_unique_ptr<Expression>(node.initializers[i]));

            auto assignment = std::make_unique<AssignmentStatement>(
                std::move(lhs_vec),
                std::move(rhs_vec)
            );
            current_basic_block->add_statement(std::move(assignment));
        }
    }
}

// --- Other visit methods for statements and expressions ---

void CFGBuilderPass::visit(AssignmentStatement& node) {
    if (!current_basic_block) end_current_block_and_start_new();
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    for (const auto& lhs_expr : node.lhs) { if(lhs_expr) lhs_expr->accept(*this); }
    for (const auto& rhs_expr : node.rhs) { if(rhs_expr) rhs_expr->accept(*this); }
}

void CFGBuilderPass::visit(RoutineCallStatement& node) {
    if (!current_basic_block) end_current_block_and_start_new();
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    if(node.routine_expr) node.routine_expr->accept(*this);
    for (const auto& arg : node.arguments) { if(arg) arg->accept(*this); }
}

void CFGBuilderPass::visit(IfStatement& node) {
    if(node.condition) node.condition->accept(*this);
    if (!current_basic_block) end_current_block_and_start_new();
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));

    BasicBlock* condition_block = current_basic_block;
    BasicBlock* then_block = create_new_basic_block("Then_");
    BasicBlock* join_block = create_new_basic_block("Join_");

    current_cfg->add_edge(condition_block, then_block);
    current_cfg->add_edge(condition_block, join_block);

    current_basic_block = then_block;
    if(node.then_branch) node.then_branch->accept(*this);
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, join_block);
    }

    current_basic_block = join_block;
}

void CFGBuilderPass::visit(ForStatement& node) {
    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(ForStatement) entered." << std::endl;

    if (!current_basic_block) end_current_block_and_start_new();

    if (node.start_expr) node.start_expr->accept(*this);

    // Create an assignment statement for the initialization
    auto lhs_init = std::make_unique<VariableAccess>(node.unique_loop_variable_name);
    std::vector<ExprPtr> lhs_vec;
    lhs_vec.push_back(std::move(lhs_init));
    std::vector<ExprPtr> rhs_vec;
    rhs_vec.push_back(clone_unique_ptr(node.start_expr));
    current_basic_block->add_statement(std::make_unique<AssignmentStatement>(std::move(lhs_vec), std::move(rhs_vec)));

    BasicBlock* init_block = current_basic_block;
    BasicBlock* header_block = create_new_basic_block("ForHeader_");
    current_cfg->add_edge(init_block, header_block);

    BasicBlock* body_block = create_new_basic_block("ForBody_");
    BasicBlock* increment_block = create_new_basic_block("ForIncrement_");
    BasicBlock* exit_block = create_new_basic_block("ForExit_");

    break_targets.push_back(exit_block);
    loop_targets.push_back(increment_block);

    header_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    current_cfg->add_edge(header_block, body_block);
    current_cfg->add_edge(header_block, exit_block);

    current_basic_block = body_block;
    if (node.body) {
        node.body->accept(*this);
    }
    if (current_basic_block) {
        current_cfg->add_edge(current_basic_block, increment_block);
    }

    current_basic_block = increment_block;
    auto lhs_incr = std::make_unique<VariableAccess>(node.unique_loop_variable_name);
    std::vector<ExprPtr> lhs_incr_vec;
    lhs_incr_vec.push_back(std::move(lhs_incr));
    std::vector<ExprPtr> rhs_incr_vec;
    auto step_val = node.step_expr ? clone_unique_ptr(node.step_expr) : std::make_unique<NumberLiteral>(static_cast<int64_t>(1));
    rhs_incr_vec.push_back(std::make_unique<BinaryOp>(
        BinaryOp::Operator::Add,
        std::make_unique<VariableAccess>(node.unique_loop_variable_name),
        std::move(step_val)
    ));
    increment_block->add_statement(std::make_unique<AssignmentStatement>(std::move(lhs_incr_vec), std::move(rhs_incr_vec)));
    current_cfg->add_edge(increment_block, header_block);

    current_basic_block = exit_block;
    loop_targets.pop_back();
    break_targets.pop_back();
    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(ForStatement) exiting." << std::endl;
}

void CFGBuilderPass::visit(ForEachStatement& node) {
    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(ForEachStatement) entered." << std::endl;



    if (!current_basic_block) end_current_block_and_start_new();

    // Determine the type of the collection expression (respects SETTYPE and inference)
    auto& analyzer = ASTAnalyzer::getInstance();
    VarType collection_type = analyzer.infer_expression_type(node.collection_expression.get());
    bool is_list = false;

    switch (collection_type) {
        case VarType::POINTER_TO_ANY_LIST:
        case VarType::CONST_POINTER_TO_ANY_LIST:
        case VarType::POINTER_TO_INT_LIST:
        case VarType::CONST_POINTER_TO_INT_LIST:
        case VarType::POINTER_TO_FLOAT_LIST:
        case VarType::CONST_POINTER_TO_FLOAT_LIST:
        case VarType::POINTER_TO_STRING_LIST:
        case VarType::CONST_POINTER_TO_STRING_LIST:
            is_list = true;
            break;
        default:
            // This covers VECTORS and STRINGS
            is_list = false;
            break;
    }

    if (is_list) {
        build_list_foreach_cfg(node);
    } else {
        build_vector_foreach_cfg(node);
    }

    // FIX: Pop the break and loop targets from the stack after the FOREACH loop
    // has been fully processed. This must be done for ALL collection types.
    if (!break_targets.empty()) {
        break_targets.pop_back();
    }
    if (!loop_targets.empty()) {
        loop_targets.pop_back();
    }

    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(ForEachStatement) exiting." << std::endl;
}


void CFGBuilderPass::visit(ReturnStatement& node) {
    if (!current_basic_block) end_current_block_and_start_new();
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    if (!current_cfg->exit_block) {
        current_cfg->exit_block = create_new_basic_block("Exit_");
        current_cfg->exit_block->is_exit = true;
    }
    current_cfg->add_edge(current_basic_block, current_cfg->exit_block);
    current_basic_block = nullptr;
}

void CFGBuilderPass::visit(ResultisStatement& node) {
    if (!current_basic_block) end_current_block_and_start_new();
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
    if (!current_cfg->exit_block) {
        current_cfg->exit_block = create_new_basic_block("Exit_");
        current_cfg->exit_block->is_exit = true;
    }
    current_cfg->add_edge(current_basic_block, current_cfg->exit_block);
    current_basic_block = nullptr;
}

void CFGBuilderPass::visit(CompoundStatement& node) {
    for (const auto& stmt : node.statements) {
        if (!stmt) continue;
        if (current_basic_block == nullptr) {
            current_basic_block = create_new_basic_block();
        }
        stmt->accept(*this);
    }
}

void CFGBuilderPass::visit(BlockStatement& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] Visiting BlockStatement." << std::endl;
    }
    for (const auto& decl : node.declarations) {
        if (current_basic_block == nullptr) {
            current_basic_block = create_new_basic_block();
        }
        if (decl) {
            decl->accept(*this);
        }
    }
    for (const auto& stmt : node.statements) {
        if (current_basic_block == nullptr) {
            current_basic_block = create_new_basic_block();
        }
        if (stmt) {
            stmt->accept(*this);
        }
    }
}

void CFGBuilderPass::visit(LabelTargetStatement& node) {
    if (trace_enabled_) {
        std::cout << "[CFGBuilderPass] Visiting LabelTargetStatement: " << node.labelName << std::endl;
    }

    BasicBlock* predecessor_block = current_basic_block;
    BasicBlock* label_block = create_new_basic_block("Label_" + node.labelName);
    label_targets[node.labelName] = label_block;

    if (predecessor_block && !predecessor_block->ends_with_control_flow()) {
        current_cfg->add_edge(predecessor_block, label_block);
    }

    current_basic_block = label_block;
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));
}

// Add stubs for other visit methods to prevent compiler warnings/errors
// These can be filled in as needed.
void CFGBuilderPass::visit(ManifestDeclaration& node) {}
void CFGBuilderPass::visit(StaticDeclaration& node) { if (node.initializer) node.initializer->accept(*this); }
void CFGBuilderPass::visit(GlobalDeclaration& node) {}
void CFGBuilderPass::visit(GlobalVariableDeclaration& node) {}
void CFGBuilderPass::visit(NumberLiteral& node) {}
void CFGBuilderPass::visit(StringLiteral& node) {}
void CFGBuilderPass::visit(CharLiteral& node) {}
void CFGBuilderPass::visit(BooleanLiteral& node) {}
void CFGBuilderPass::visit(VariableAccess& node) {}
void CFGBuilderPass::visit(BinaryOp& node) { if(node.left) node.left->accept(*this); if(node.right) node.right->accept(*this); }
void CFGBuilderPass::visit(UnaryOp& node) { if(node.operand) node.operand->accept(*this); }
void CFGBuilderPass::visit(VectorAccess& node) { if(node.vector_expr) node.vector_expr->accept(*this); if(node.index_expr) node.index_expr->accept(*this); }
void CFGBuilderPass::visit(CharIndirection& node) { if(node.string_expr) node.string_expr->accept(*this); if(node.index_expr) node.index_expr->accept(*this); }
void CFGBuilderPass::visit(FloatVectorIndirection& node) { if(node.vector_expr) node.vector_expr->accept(*this); if(node.index_expr) node.index_expr->accept(*this); }
void CFGBuilderPass::visit(FunctionCall& node) { if(node.function_expr) node.function_expr->accept(*this); for(const auto& arg : node.arguments) { if(arg) arg->accept(*this); } }
void CFGBuilderPass::visit(ConditionalExpression& node) { if(node.condition) node.condition->accept(*this); if(node.true_expr) node.true_expr->accept(*this); if(node.false_expr) node.false_expr->accept(*this); }
void CFGBuilderPass::visit(ValofExpression& node) { if(node.body) node.body->accept(*this); }
void CFGBuilderPass::visit(FloatValofExpression& node) { if(node.body) node.body->accept(*this); }
void CFGBuilderPass::visit(UnlessStatement& node) { /* Similar to IfStatement */ }
void CFGBuilderPass::visit(TestStatement& node) { /* Similar to IfStatement with else */ }
void CFGBuilderPass::visit(WhileStatement& node) { /* Similar to ForStatement */ }
void CFGBuilderPass::visit(UntilStatement& node) { /* Similar to WhileStatement */ }
void CFGBuilderPass::visit(RepeatStatement& node) { /* Loop construct */ }
void CFGBuilderPass::visit(SwitchonStatement& node) {
    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(SwitchonStatement) entered." << std::endl;

    // --- START OF FIX ---
    // Finalize the current block (which contains the FOREACH setup) and start a new,
    // dedicated block for the SWITCHON's branching logic.
    end_current_block_and_start_new();
    // --- END OF FIX ---

    // 1. Add a clone of the SWITCHON node to the new, current block.
    current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release())));

    // 2. Create the necessary blocks for control flow.
    BasicBlock* switch_header_block = current_basic_block;
    BasicBlock* join_block = create_new_basic_block("SwitchJoin_");
    endcase_targets.push_back(join_block); // Register the join block for ENDCASE statements.

    std::vector<BasicBlock*> case_blocks;
    for (size_t i = 0; i < node.cases.size(); ++i) {
        case_blocks.push_back(create_new_basic_block("Case_" + std::to_string(i) + "_"));
    }

    BasicBlock* default_block = nullptr;
    if (node.default_case) {
        default_block = create_new_basic_block("DefaultCase_");
    }

    // 3. Add edges from the header to all possible branches.
    for (BasicBlock* case_block : case_blocks) {
        current_cfg->add_edge(switch_header_block, case_block);
    }
    if (default_block) {
        current_cfg->add_edge(switch_header_block, default_block);
    }
    // The final successor is the join block, which acts as the default if no default case exists.
    current_cfg->add_edge(switch_header_block, join_block);

    // 4. Visit the command within each case, setting the current block appropriately.
    for (size_t i = 0; i < node.cases.size(); ++i) {
        current_basic_block = case_blocks[i];
        if (node.cases[i]->command) {
            node.cases[i]->command->accept(*this);
        }
        // If a case doesn't end with a GOTO, RETURN, or ENDCASE, it must fall through to the join block.
        if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
            current_cfg->add_edge(current_basic_block, join_block);
        }
    }

    // 5. Visit the default case command.
    if (default_block) {
        current_basic_block = default_block;
        if (node.default_case->command) {
            node.default_case->command->accept(*this);
        }
        if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
            current_cfg->add_edge(current_basic_block, join_block);
        }
    }

    // 6. The new current block for any code following the switch is the join block.
    current_basic_block = join_block;
    endcase_targets.pop_back();

    if (trace_enabled_) std::cout << "[CFGBuilderPass] visit(SwitchonStatement) exiting." << std::endl;
}
void CFGBuilderPass::visit(CaseStatement& node) {}
void CFGBuilderPass::visit(DefaultStatement& node) {}
void CFGBuilderPass::visit(GotoStatement& node) { if (!current_basic_block) end_current_block_and_start_new(); current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); unresolved_gotos_.push_back({&node, current_basic_block}); current_basic_block = nullptr; }
void CFGBuilderPass::visit(FinishStatement& node) { if (!current_basic_block) end_current_block_and_start_new(); current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); current_basic_block = nullptr; }
void CFGBuilderPass::visit(BreakStatement& node) { if (!current_basic_block) end_current_block_and_start_new(); current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); if (!break_targets.empty()) current_cfg->add_edge(current_basic_block, break_targets.back()); current_basic_block = nullptr; }
void CFGBuilderPass::visit(LoopStatement& node) { if (!current_basic_block) end_current_block_and_start_new(); current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); if (!loop_targets.empty()) current_cfg->add_edge(current_basic_block, loop_targets.back()); current_basic_block = nullptr; }
void CFGBuilderPass::visit(EndcaseStatement& node) { if (!current_basic_block) end_current_block_and_start_new(); current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); if (!endcase_targets.empty()) current_cfg->add_edge(current_basic_block, endcase_targets.back()); current_basic_block = nullptr; }
void CFGBuilderPass::visit(StringStatement& node) { if (!current_basic_block) end_current_block_and_start_new(); current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); if(node.size_expr) node.size_expr->accept(*this); }
void CFGBuilderPass::visit(BrkStatement& node) { if (!current_basic_block) end_current_block_and_start_new(); current_basic_block->add_statement(std::unique_ptr<Statement>(static_cast<Statement*>(node.clone().release()))); }
void CFGBuilderPass::visit(FreeStatement& node) {}
void CFGBuilderPass::visit(ConditionalBranchStatement& node) {}
void CFGBuilderPass::visit(SysCall& node) {}
void CFGBuilderPass::visit(VecAllocationExpression& node) {}
// void CFGBuilderPass::visit(FVecAllocationExpression& node) {} // Commented out
void CFGBuilderPass::visit(StringAllocationExpression& node) {}
void CFGBuilderPass::visit(TableExpression& node) {}
// void CFGBuilderPass::visit(BitfieldAccessExpression& node) {} // Commented out
// void CFGBuilderPass::visit(ListExpression& node) {} // Commented out
void CFGBuilderPass::visit(LabelDeclaration& node) {}


void CFGBuilderPass::build_vector_foreach_cfg(ForEachStatement& node) {
    if (trace_enabled_) std::cout << "[CFGBuilderPass] Building CFG for vector-based FOREACH." << std::endl;

    // --- Step 1: Create unique names for loop control variables ---
    std::string collection_name = "_forEach_vec_" + std::to_string(block_id_counter++);
    std::string len_name = "_forEach_len_" + std::to_string(block_id_counter++);
    std::string index_name = "_forEach_idx_" + std::to_string(block_id_counter++);

    // --- Step 2: Register these new temporary variables with the analyzer ---
    // This ensures the code generator knows about them and allocates space.
    auto& analyzer = ASTAnalyzer::getInstance();
    if (current_cfg && !current_cfg->function_name.empty()) {
        auto& metrics = analyzer.get_function_metrics_mut().at(current_cfg->function_name);
        metrics.variable_types[collection_name] = analyzer.infer_expression_type(node.collection_expression.get());
        metrics.variable_types[len_name] = VarType::INTEGER;
        metrics.variable_types[index_name] = VarType::INTEGER;
        metrics.num_variables += 3; // For collection, length, and index
    }

    // --- Step 3: Populate the Pre-Header block with initialization code ---
    // This code runs once before the loop begins.
    if (!current_basic_block) end_current_block_and_start_new();

    // LET _collection = <original collection expression>
    {
        std::vector<ExprPtr> collection_rhs;
        collection_rhs.push_back(clone_unique_ptr(node.collection_expression));
        current_basic_block->add_statement(std::make_unique<LetDeclaration>(
            std::vector<std::string>{collection_name},
            std::move(collection_rhs)
        ));
    }

    // LET _len = LEN(_collection)
    {
        std::vector<ExprPtr> len_rhs;
        len_rhs.push_back(std::make_unique<UnaryOp>(UnaryOp::Operator::LengthOf, std::make_unique<VariableAccess>(collection_name)));
        current_basic_block->add_statement(std::make_unique<LetDeclaration>(
            std::vector<std::string>{len_name},
            std::move(len_rhs)
        ));
    }

    // LET _idx = 0
    {
        std::vector<ExprPtr> idx_rhs;
        idx_rhs.push_back(std::make_unique<NumberLiteral>(static_cast<int64_t>(0)));
        current_basic_block->add_statement(std::make_unique<LetDeclaration>(
            std::vector<std::string>{index_name},
            std::move(idx_rhs)
        ));
    }

    // --- Step 4: Create the core basic blocks for the loop structure ---
    BasicBlock* header_block = create_new_basic_block("ForEachHeader_");
    BasicBlock* body_block = create_new_basic_block("ForEachBody_");
    BasicBlock* increment_block = create_new_basic_block("ForEachIncrement_");
    BasicBlock* exit_block = create_new_basic_block("ForEachExit_");

    // --- Step 5: Connect the blocks and populate them ---

    // The initialization block flows into the loop header
    current_cfg->add_edge(current_basic_block, header_block);

    // Populate the header block (the loop condition check)
    // Condition: IF _idx >= _len GOTO exit_block
    auto condition = std::make_unique<BinaryOp>(
        BinaryOp::Operator::GreaterEqual,
        std::make_unique<VariableAccess>(index_name),
        std::make_unique<VariableAccess>(len_name)
    );
    auto conditional_branch = std::make_unique<ConditionalBranchStatement>("NE", exit_block->id, std::move(condition));
    header_block->add_statement(std::move(conditional_branch));

    // The header branches to the body (if condition false) or the exit (if condition true)
    current_cfg->add_edge(header_block, body_block);
    current_cfg->add_edge(header_block, exit_block);

    // Populate the body block
    current_basic_block = body_block;

    // First statement in the body is to get the element: V := _collection ! _idx
    VarType collection_type = analyzer.infer_expression_type(node.collection_expression.get());
    ExprPtr access_expr;
    if (collection_type == VarType::POINTER_TO_STRING) {
        access_expr = std::make_unique<CharIndirection>(std::make_unique<VariableAccess>(collection_name), std::make_unique<VariableAccess>(index_name));
    } else if (collection_type == VarType::POINTER_TO_FLOAT_VEC) {
        access_expr = std::make_unique<FloatVectorIndirection>(std::make_unique<VariableAccess>(collection_name), std::make_unique<VariableAccess>(index_name));
    } else {
        access_expr = std::make_unique<VectorAccess>(std::make_unique<VariableAccess>(collection_name), std::make_unique<VariableAccess>(index_name));
    }
    std::vector<ExprPtr> lhs_vec;
    lhs_vec.push_back(std::make_unique<VariableAccess>(node.loop_variable_name));
    std::vector<ExprPtr> rhs_vec;
    rhs_vec.push_back(std::move(access_expr));
    current_basic_block->add_statement(std::make_unique<AssignmentStatement>(
        std::move(lhs_vec),
        std::move(rhs_vec)
    ));

    // Set up break/loop targets and then process the user's original loop body
    break_targets.push_back(exit_block);
    loop_targets.push_back(increment_block);
    if (node.body) {
        node.body->accept(*this);
    }

    // The user's code flows into the increment block (unless it had a BREAK, LOOP, etc.)
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, increment_block);
    }
    
    // Populate the increment block
    current_basic_block = increment_block;
    std::vector<ExprPtr> lhs_vec2;
    lhs_vec2.push_back(std::make_unique<VariableAccess>(index_name));
    std::vector<ExprPtr> rhs_vec2;
    rhs_vec2.push_back(std::make_unique<BinaryOp>(
        BinaryOp::Operator::Add,
        std::make_unique<VariableAccess>(index_name),
        std::make_unique<NumberLiteral>(static_cast<int64_t>(1))
    ));
    auto increment_stmt = std::make_unique<AssignmentStatement>(
        std::move(lhs_vec2),
        std::move(rhs_vec2)
    );
    increment_block->add_statement(std::move(increment_stmt));

    // The increment block unconditionally loops back to the header
    current_cfg->add_edge(increment_block, header_block);

    // --- Step 6: Finalize ---
    // Code generation will now continue from the exit block.
    current_basic_block = exit_block;
    // Clean up the loop context stacks
    break_targets.pop_back();
    loop_targets.pop_back();

    if (trace_enabled_) std::cout << "[CFGBuilderPass] Correctly built low-level CFG for vector-based FOREACH." << std::endl;
}

void CFGBuilderPass::build_list_foreach_cfg(ForEachStatement& node) {
    // --- Pre-header: Initialization ---
    // 1. Create unique name for our iterator
    std::string cursor_name = "_forEach_cursor_" + std::to_string(block_id_counter++);

    // --- Register the cursor variable with ASTAnalyzer ---
    auto& analyzer = ASTAnalyzer::getInstance();
    auto& metrics = analyzer.get_function_metrics_mut().at(current_cfg->function_name);
    metrics.variable_types[cursor_name] = VarType::POINTER_TO_LIST_NODE;
    metrics.num_variables++;

    // 2. Initialize the cursor to the first node of the list: LET _cursor = *(&L + 16)
    std::vector<ExprPtr> cursor_rhs;
    // Correct: Initialize cursor to the first node (header->head) using indirection at offset 16.
    cursor_rhs.push_back(
        std::make_unique<UnaryOp>(
            UnaryOp::Operator::Indirection,
            std::make_unique<BinaryOp>(
                BinaryOp::Operator::Add,
                clone_unique_ptr(node.collection_expression),
                std::make_unique<NumberLiteral>(static_cast<int64_t>(16))
            )
        )
    );
    auto init_cursor_stmt = std::make_unique<LetDeclaration>(
        std::vector<std::string>{cursor_name},
        std::move(cursor_rhs)
    );
    current_basic_block->add_statement(std::move(init_cursor_stmt));

    // --- Loop Header: Condition Check ---
    BasicBlock* header_block = create_new_basic_block("ForEachHeader_");
    BasicBlock* body_block = create_new_basic_block("ForEachBody_");
    BasicBlock* exit_block = create_new_basic_block("ForEachExit_");

    // Link the pre-header to the loop header
    current_cfg->add_edge(current_basic_block, header_block);

    // Conditional branch: IF _cursor = 0 GOTO exit_block
    auto condition_check = std::make_unique<ConditionalBranchStatement>(
        "EQ", // Branch if Equal to zero
        exit_block->id,
        std::make_unique<VariableAccess>(cursor_name)
    );
    header_block->add_statement(std::move(condition_check));

    // Add the control flow edges from the header
    current_cfg->add_edge(header_block, body_block);   // If condition fails (cursor is not 0), go to body.
    current_cfg->add_edge(header_block, exit_block);   // If condition succeeds (cursor is 0), go to exit.

    // --- Loop Body: Assign Value and Execute User Code ---
    current_basic_block = body_block;

    // At the top of the body, assign the type tag to the type variable if present
    if (!node.type_variable_name.empty()) {
        // LET T = TYPEOF(cursor)
        auto typeof_expr = std::make_unique<UnaryOp>(UnaryOp::Operator::TypeOf, std::make_unique<VariableAccess>(cursor_name));
        std::vector<ExprPtr> typeof_rhs;
        typeof_rhs.push_back(std::move(typeof_expr));
        auto let_type = std::make_unique<LetDeclaration>(
            std::vector<std::string>{node.type_variable_name},
            std::move(typeof_rhs)
        );
        current_basic_block->add_statement(std::move(let_type));
    }

    // --- STEP 1: Assign the value to the loop variable FIRST ---
    if (!node.type_variable_name.empty()) {
        // Two-variable FOREACH: X is the pointer to the atom, not its value.
        std::vector<ExprPtr> lhs;
        lhs.push_back(std::make_unique<VariableAccess>(node.loop_variable_name));
        std::vector<ExprPtr> rhs;
        rhs.push_back(std::make_unique<VariableAccess>(cursor_name));
        auto assign_value_stmt = std::make_unique<AssignmentStatement>(
            std::move(lhs),
            std::move(rhs)
        );
        current_basic_block->add_statement(std::move(assign_value_stmt));
    } else {
        // One-variable form: S = HD(_cursor)
        std::vector<ExprPtr> lhs;
        lhs.push_back(std::make_unique<VariableAccess>(node.loop_variable_name));
        std::vector<ExprPtr> rhs;
        rhs.push_back(std::make_unique<UnaryOp>(
            node.inferred_element_type == VarType::FLOAT ? UnaryOp::Operator::HeadOfAsFloat : UnaryOp::Operator::HeadOf,
            std::make_unique<VariableAccess>(cursor_name)
        ));
        auto assign_value_stmt = std::make_unique<AssignmentStatement>(
            std::move(lhs),
            std::move(rhs)
        );
        current_basic_block->add_statement(std::move(assign_value_stmt));

        // If it's a string, also add the pointer adjustment
        if (node.inferred_element_type == VarType::POINTER_TO_STRING) {
            std::vector<ExprPtr> lhs_adj;
            lhs_adj.push_back(std::make_unique<VariableAccess>(node.loop_variable_name));
            std::vector<ExprPtr> rhs_adj;
            rhs_adj.push_back(std::make_unique<BinaryOp>(
                BinaryOp::Operator::Add,
                std::make_unique<VariableAccess>(node.loop_variable_name),
                std::make_unique<NumberLiteral>(static_cast<int64_t>(8))
            ));
            auto adjust_stmt = std::make_unique<AssignmentStatement>(std::move(lhs_adj), std::move(rhs_adj));
            current_basic_block->add_statement(std::move(adjust_stmt));
        }
    }

    // --- STEP 2: Process the user's original loop body ---
    if (node.body) {
        node.body->accept(*this);
    }

    // --- STEP 3: Create the advance block and link to it ---
    BasicBlock* advance_block = create_new_basic_block("ForEachAdvance_");
    if (current_basic_block && !current_basic_block->ends_with_control_flow()) {
        current_cfg->add_edge(current_basic_block, advance_block);
    }

    // --- STEP 4: In the new block, advance the cursor and loop back ---
    current_basic_block = advance_block;
    std::vector<ExprPtr> lhs2;
    lhs2.push_back(std::make_unique<VariableAccess>(cursor_name));
    std::vector<ExprPtr> rhs2;
    rhs2.push_back(std::make_unique<UnaryOp>(
        UnaryOp::Operator::TailOfNonDestructive,
        std::make_unique<VariableAccess>(cursor_name)
    ));
    auto advance_cursor_stmt = std::make_unique<AssignmentStatement>(
        std::move(lhs2),
        std::move(rhs2)
    );
    current_basic_block->add_statement(std::move(advance_cursor_stmt));
    current_cfg->add_edge(current_basic_block, header_block);

    // --- Loop Exit ---
    // Set the current block for code generation to the exit block
    current_basic_block = exit_block;

    // Push the exit block onto the break/loop stacks so BREAK/LOOP statements work
    break_targets.push_back(exit_block);
    loop_targets.push_back(header_block);

    // Remember to pop them after the loop context is fully parsed
    // (Handled in visit(ForEachStatement))
}
