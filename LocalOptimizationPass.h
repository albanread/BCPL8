#pragma once

#include "ControlFlowGraph.h"
#include <unordered_map>
#include <string>
#include <memory>
#include "SymbolTable.h"
#include "analysis/ASTAnalyzer.h"
#include "DataTypes.h"

// Forward declaration for AST nodes
class AssignmentStatement;
class Stmt;


/**
 * LocalOptimizationPass
 * 
 * Performs local optimizations (such as Common Subexpression Elimination)
 * within each basic block of the Control Flow Graph (CFG).
 * 
 * Usage:
 *   LocalOptimizationPass pass;
 *   pass.run(cfgs); // where cfgs is the map from function name to ControlFlowGraph
 */
class LocalOptimizationPass {
public:
    LocalOptimizationPass();

    // Run local optimizations on all functions/routines in the program.
    void run(const std::unordered_map<std::string, std::unique_ptr<ControlFlowGraph>>& cfgs,
             SymbolTable& symbol_table,
             ASTAnalyzer& analyzer);

private:
    // Map from canonical expression string to temporary variable name
    std::unordered_map<std::string, std::string> available_expressions_;
    int temp_var_counter_;

    // --- NEW: Map from canonical expression string to count for analysis stage ---
    std::unordered_map<std::string, int> expr_counts_;

    // Generate a unique temporary variable name for hoisted expressions
    std::string generate_temp_var_name();

    // Infer the type of an expression (INTEGER or FLOAT)
    VarType infer_expression_type(const Expression* expr);

    // Apply CSE and other local optimizations to a single basic block
    void optimize_basic_block(BasicBlock* block,
                             const std::string& function_name,
                             SymbolTable& symbol_table,
                             ASTAnalyzer& analyzer);

    // CSE logic for a single assignment statement, allows statement insertion
    void optimize_assignment(BasicBlock* block, size_t& i,
                             const std::string& current_function_name,
                             SymbolTable& symbol_table,
                             ASTAnalyzer& analyzer);

    // Helper: Invalidate expressions that use a given variable name
    void invalidate_expressions_with_var(const std::string& var_name);

    // Helper: Get the canonical string for an expression (for CSE)
    std::string expression_to_string(const Expression* expr) const;

    // --- NEW: Helper to count subexpressions for analysis stage ---
    void count_subexpressions(ASTNode* node);
};
