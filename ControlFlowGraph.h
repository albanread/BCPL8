#ifndef CONTROL_FLOW_GRAPH_H
#define CONTROL_FLOW_GRAPH_H

#include "BasicBlock.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

class ControlFlowGraph {
public:
    std::unordered_map<std::string, std::unique_ptr<BasicBlock>> blocks; // Owns all basic blocks in the graph
    BasicBlock* entry_block; // Pointer to the entry basic block
    BasicBlock* exit_block;  // Pointer to a conceptual exit block (useful for analysis)
    std::string function_name; // The name of the function/routine this CFG belongs to

    // Constructor
    ControlFlowGraph(std::string func_name);

    // Factory method to create and add a new basic block
    BasicBlock* create_block(const std::string& id_prefix = "BB_");

    // Adds a control flow edge between two blocks
    void add_edge(BasicBlock* from, BasicBlock* to);

    // Retrieves a block by its ID
    BasicBlock* get_block(const std::string& id) const;

    // Debugging: Print the CFG structure
    void print_cfg() const;

    // Accessor for all basic blocks (needed for register pressure calculation)
    const std::unordered_map<std::string, std::unique_ptr<BasicBlock>>& get_blocks() const { return blocks; }

private:
    int block_id_counter_;
};

#endif // CONTROL_FLOW_GRAPH_H
