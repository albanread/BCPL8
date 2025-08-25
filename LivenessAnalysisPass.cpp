#include "LivenessAnalysisPass.h"
#include "AST.h"
#include <iostream>

// Constructor
LivenessAnalysisPass::LivenessAnalysisPass(const CFGMap& cfgs, bool trace_enabled) 
    : cfgs_(cfgs), trace_enabled_(trace_enabled) {}

// Phase 1: Compute use and def sets for every basic block in every CFG.
// Implementation moved to separate file: live_compute_use_def_sets.cpp

// Analyze a single basic block to populate its use and def sets.
// Implementation moved to separate file: live_analyze_block.cpp



// Visitor for variable access (a 'use' case).
void LivenessAnalysisPass::visit(VariableAccess& node) {
    // A variable is used if it's not already defined within this block.
    if (current_def_set_.find(node.name) == current_def_set_.end()) {
        current_use_set_.insert(node.name);
    }
}

// Visitor for assignment (a 'def' case).
void LivenessAnalysisPass::visit(AssignmentStatement& node) {
    // First, visit the RHS to find all 'uses'.
    for (const auto& rhs_expr : node.rhs) {
        rhs_expr->accept(*this);
    }

    // Then, visit the LHS to find all 'defs'.
    for (const auto& lhs_expr : node.lhs) {
        if (auto* var = dynamic_cast<VariableAccess*>(lhs_expr.get())) {
            // This variable is defined here.
            current_def_set_.insert(var->name);
        }
    }
}

// --- Recursive ASTVisitor overrides for liveness analysis ---

void LivenessAnalysisPass::visit(BinaryOp& node) {
    if (node.left) node.left->accept(*this);
    if (node.right) node.right->accept(*this);
}

void LivenessAnalysisPass::visit(UnaryOp& node) {
    if (node.operand) node.operand->accept(*this);
}

void LivenessAnalysisPass::visit(FunctionCall& node) {
    if (node.function_expr) node.function_expr->accept(*this);
    for (const auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}

void LivenessAnalysisPass::visit(VectorAccess& node) {
    if (node.vector_expr) node.vector_expr->accept(*this);
    if (node.index_expr) node.index_expr->accept(*this);
}

void LivenessAnalysisPass::visit(CharIndirection& node) {
    if (node.string_expr) node.string_expr->accept(*this);
    if (node.index_expr) node.index_expr->accept(*this);
}

void LivenessAnalysisPass::visit(FloatVectorIndirection& node) {
    if (node.vector_expr) node.vector_expr->accept(*this);
    if (node.index_expr) node.index_expr->accept(*this);
}

void LivenessAnalysisPass::visit(ConditionalExpression& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.true_expr) node.true_expr->accept(*this);
    if (node.false_expr) node.false_expr->accept(*this);
}

void LivenessAnalysisPass::visit(ValofExpression& node) {
    if (!node.body) return;

    // Enter the VALOF block and analyze its statements
    node.body->accept(*this);

    // Ensure any variables declared or used in the VALOF block are analyzed
    if (trace_enabled_) {
        std::cout << "[DEBUG] Analyzing VALOF block for liveness." << std::endl;
    }
}

void LivenessAnalysisPass::visit(RoutineCallStatement& node) {
    if (node.routine_expr) node.routine_expr->accept(*this);
    for (const auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}

void LivenessAnalysisPass::visit(IfStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.then_branch) node.then_branch->accept(*this);
}

void LivenessAnalysisPass::visit(UnlessStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.then_branch) node.then_branch->accept(*this);
}

void LivenessAnalysisPass::visit(TestStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.then_branch) node.then_branch->accept(*this);
    if (node.else_branch) node.else_branch->accept(*this);
}

void LivenessAnalysisPass::visit(WhileStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void LivenessAnalysisPass::visit(UntilStatement& node) {
    if (node.condition) node.condition->accept(*this);
    if (node.body) node.body->accept(*this);
}

void LivenessAnalysisPass::visit(RepeatStatement& node) {
    if (node.body) node.body->accept(*this);
}

void LivenessAnalysisPass::visit(ForStatement& node) {
    if (node.start_expr) node.start_expr->accept(*this);
    if (node.end_expr) node.end_expr->accept(*this);
    if (node.step_expr) node.step_expr->accept(*this);
    if (node.body) node.body->accept(*this);
}

void LivenessAnalysisPass::visit(SwitchonStatement& node) {
    if (node.expression) node.expression->accept(*this);
    for (const auto& case_stmt : node.cases) {
        if (case_stmt) case_stmt->accept(*this);
    }
    if (node.default_case) node.default_case->accept(*this);
}

void LivenessAnalysisPass::visit(CaseStatement& node) {
    if (node.constant_expr) node.constant_expr->accept(*this);
    if (node.command) node.command->accept(*this);
}

void LivenessAnalysisPass::visit(DefaultStatement& node) {
    if (node.command) node.command->accept(*this);
}

void LivenessAnalysisPass::visit(GotoStatement& node) {
    // No variable use in label jump
}

void LivenessAnalysisPass::visit(ReturnStatement& node) {
    // No return value expression in ReturnStatement
}

void LivenessAnalysisPass::visit(FinishStatement& node) {
    if (node.syscall_number) node.syscall_number->accept(*this);
    for (const auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}

void LivenessAnalysisPass::visit(BreakStatement& node) {
    // No variable use
}

void LivenessAnalysisPass::visit(LoopStatement& node) {
    // No body field in LoopStatement
}

void LivenessAnalysisPass::visit(EndcaseStatement& node) {
    // No variable use
}

void LivenessAnalysisPass::visit(FreeStatement& node) {
    if (node.list_expr) node.list_expr->accept(*this);
}

void LivenessAnalysisPass::visit(CompoundStatement& node) {
    for (const auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void LivenessAnalysisPass::visit(BlockStatement& node) {
    for (const auto& stmt : node.statements) {
        if (stmt) stmt->accept(*this);
    }
}

void LivenessAnalysisPass::visit(StringStatement& node) {
    // No variable use
    }

    void LivenessAnalysisPass::visit(ResultisStatement& node) {
        if (node.expression) node.expression->accept(*this);
    }



void LivenessAnalysisPass::visit(LabelTargetStatement& node) {
    // LabelTargetStatement only has a labelName (no statement/command to visit)
}

void LivenessAnalysisPass::visit(ConditionalBranchStatement& node) {
    if (node.condition_expr) node.condition_expr->accept(*this);
    // node.targetLabel is a string, not an AST node
}

void LivenessAnalysisPass::visit(SysCall& node) {
    for (const auto& arg : node.arguments) {
        if (arg) arg->accept(*this);
    }
}

void LivenessAnalysisPass::visit(VecAllocationExpression& node) {
    if (node.size_expr) node.size_expr->accept(*this);
}

void LivenessAnalysisPass::visit(StringAllocationExpression& node) {
    if (node.size_expr) node.size_expr->accept(*this);
}

void LivenessAnalysisPass::visit(TableExpression& node) {
    for (const auto& entry : node.initializers) {
        if (entry) entry->accept(*this);
    }
}

void LivenessAnalysisPass::visit(GlobalVariableDeclaration& node) {
    // Global variables are not part of liveness analysis within functions.
    // This method is implemented to satisfy the ASTVisitor interface.
    
    // If needed, we could process initializers here:
    // for (const auto& initializer : node.initializers) {
    //     if (initializer) initializer->accept(*this);
    // }
}

// Phase 2: Run the iterative data-flow algorithm.
// Implementation moved to separate file: live_run_data_flow_analysis.cpp

// Main entry point to run the analysis.
// Implementation moved to separate file: live_run.cpp

// Accessors
// Implementation moved to separate files: live_get_in_set.cpp and live_get_out_set.cpp

// Print results
// Implementation moved to separate file: live_print_results.cpp





// Print results for debugging

#include <algorithm>
#include <map>
#include <string>

std::map<std::string, int> LivenessAnalysisPass::calculate_register_pressure() const {
    std::map<std::string, int> pressure_map;

    // Iterate over each function's Control Flow Graph
    for (const auto& cfg_pair : cfgs_) {
        const std::string& func_name = cfg_pair.first;
        const auto& cfg = cfg_pair.second;
        int max_pressure = 0;

        // Iterate over each basic block in the current function's CFG
        for (const auto& block_pair : cfg->get_blocks()) {
            BasicBlock* block = block_pair.second.get();

            // Find the size of the in-set and out-set for the current block
            size_t in_size = get_in_set(block).size();
            size_t out_size = get_out_set(block).size();

            // The register pressure at this block's boundaries is the max of the two
            int current_max = std::max(in_size, out_size);

            // Keep track of the highest pressure seen so far in this function
            if (current_max > max_pressure) {
                max_pressure = current_max;
            }
        }
        pressure_map[func_name] = max_pressure;
    }
    return pressure_map;
}
