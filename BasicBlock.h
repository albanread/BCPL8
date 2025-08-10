#ifndef BASIC_BLOCK_H
#define BASIC_BLOCK_H

#include "AST.h" // For StmtPtr
#include <string>
#include <vector>
#include <memory>

// Forward declaration for ControlFlowGraph
class ControlFlowGraph;

class BasicBlock {
public:
    std::string id; // Unique identifier for the basic block (e.g., "BB_0")
    std::vector<StmtPtr> statements; // AST statements contained within this basic block
    std::vector<BasicBlock*> successors; // Pointers to basic blocks that can be executed immediately after this block
    std::vector<BasicBlock*> predecessors; // Pointers to basic blocks that can transfer control to this block
    bool is_entry; // True if this is the entry block of a function/routine
    bool is_exit; // True if this is an exit block of a function/routine
    std::string label_name; // If this block starts with a label, its name

    // Constructor
    BasicBlock(std::string id, bool is_entry = false, bool is_exit = false, std::string label_name = "");

    // Add a statement to the basic block
    void add_statement(StmtPtr stmt);

    // Add a successor to this basic block
    void add_successor(BasicBlock* successor);

    // Add a predecessor to this basic block
    void add_predecessor(BasicBlock* predecessor);

    // Check if the block ends with a control flow statement (e.g., GOTO, RETURN)
    bool ends_with_control_flow() const;
};

#endif // BASIC_BLOCK_H
