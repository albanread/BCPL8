#include "ControlFlowGraph.h"
#include <iostream>
#include <vector>
#include <unordered_set>
#include <algorithm>

// Accessor for all basic blocks (needed for register pressure calculation)

#include <sstream>

// Helper function to convert NodeType to string for printing
std::string to_string(ASTNode::NodeType type) {
    switch (type) {
        case ASTNode::NodeType::FreeStmt:
            // Handle FreeStmt case
            break;
        case ASTNode::NodeType::Program: return "Program";
        case ASTNode::NodeType::LetDecl: return "LetDecl";
        case ASTNode::NodeType::ManifestDecl: return "ManifestDecl";
        case ASTNode::NodeType::StaticDecl: return "StaticDecl";
        case ASTNode::NodeType::GlobalDecl: return "GlobalDecl";
        case ASTNode::NodeType::FunctionDecl: return "FunctionDecl";
        case ASTNode::NodeType::RoutineDecl: return "RoutineDecl";
        case ASTNode::NodeType::LabelDecl: return "LabelDecl";
        case ASTNode::NodeType::BrkStatement: return "BrkStatement";
        case ASTNode::NodeType::NumberLit: return "NumberLit";
        case ASTNode::NodeType::StringLit: return "StringLit";
        case ASTNode::NodeType::CharLit: return "CharLit";
        case ASTNode::NodeType::BooleanLit: return "BooleanLit";
        case ASTNode::NodeType::VariableAccessExpr: return "VariableAccessExpr";
        case ASTNode::NodeType::BinaryOpExpr: return "BinaryOpExpr";
        case ASTNode::NodeType::UnaryOpExpr: return "UnaryOpExpr";
        case ASTNode::NodeType::VectorAccessExpr: return "VectorAccessExpr";
        case ASTNode::NodeType::CharIndirectionExpr: return "CharIndirectionExpr";
        case ASTNode::NodeType::FloatVectorIndirectionExpr: return "FloatVectorIndirectionExpr";
        case ASTNode::NodeType::BitfieldAccessExpr: return "BitfieldAccessExpr";
        case ASTNode::NodeType::FunctionCallExpr: return "FunctionCallExpr";
        case ASTNode::NodeType::ListExpr: return "ListExpr";
        case ASTNode::NodeType::ConditionalExpr: return "ConditionalExpr";
        case ASTNode::NodeType::ValofExpr: return "ValofExpr";
        case ASTNode::NodeType::FloatValofExpr: return "FloatValofExpr";
        case ASTNode::NodeType::VecAllocationExpr: return "VecAllocationExpr";
        case ASTNode::NodeType::FVecAllocationExpr: return "FVecAllocationExpr";
        case ASTNode::NodeType::StringAllocationExpr: return "StringAllocationExpr";
        case ASTNode::NodeType::TableExpr: return "TableExpr";
        case ASTNode::NodeType::VecInitializerExpr: return "VecInitializerExpr";
        case ASTNode::NodeType::AssignmentStmt: return "AssignmentStmt";
        case ASTNode::NodeType::RoutineCallStmt: return "RoutineCallStmt";
        case ASTNode::NodeType::IfStmt: return "IfStmt";
        case ASTNode::NodeType::UnlessStmt: return "UnlessStmt";
        case ASTNode::NodeType::TestStmt: return "TestStmt";
        case ASTNode::NodeType::WhileStmt: return "WhileStmt";
        case ASTNode::NodeType::UntilStmt: return "UntilStmt";
        case ASTNode::NodeType::RepeatStmt: return "RepeatStmt";
        case ASTNode::NodeType::ForStmt: return "ForStmt";
        case ASTNode::NodeType::ForEachStmt: return "ForEachStmt";
        case ASTNode::NodeType::SwitchonStmt: return "SwitchonStmt";
        case ASTNode::NodeType::CaseStmt: return "CaseStmt";
        case ASTNode::NodeType::DefaultStmt: return "DefaultStmt";
        case ASTNode::NodeType::GotoStmt: return "GotoStmt";
        case ASTNode::NodeType::ReturnStmt: return "ReturnStmt";
        case ASTNode::NodeType::FinishStmt: return "FinishStmt";
        case ASTNode::NodeType::BreakStmt: return "BreakStmt";
        case ASTNode::NodeType::LoopStmt: return "LoopStmt";
        case ASTNode::NodeType::SysCallExpr: return "SysCallExpr";
        case ASTNode::NodeType::LabelTargetStmt: return "LabelTargetStmt";
        case ASTNode::NodeType::ConditionalBranchStmt: return "ConditionalBranchStmt";
        case ASTNode::NodeType::EndcaseStmt: return "EndcaseStmt";
        case ASTNode::NodeType::ResultisStmt: return "ResultisStmt";
        case ASTNode::NodeType::CompoundStmt: return "CompoundStmt";
        case ASTNode::NodeType::BlockStmt: return "BlockStmt";
        case ASTNode::NodeType::StringStmt: return "StringStmt";
        case ASTNode::NodeType::Declaration: return "Declaration (Base)";
        case ASTNode::NodeType::Expression: return "Expression (Base)";
        case ASTNode::NodeType::Statement: return "Statement (Base)";
        // Removed FreeListStmt case (no longer needed)
    }
    return "Unknown NodeType";
}

ControlFlowGraph::ControlFlowGraph(std::string func_name)
    : function_name(std::move(func_name)),
      entry_block(nullptr),
      exit_block(nullptr),
      block_id_counter_(0) {}

BasicBlock* ControlFlowGraph::create_block(const std::string& id_prefix) {
    std::string id = function_name + "_" + id_prefix + std::to_string(block_id_counter_++);
    auto new_block = std::make_unique<BasicBlock>(id);
    BasicBlock* raw_ptr = new_block.get();
    blocks[id] = std::move(new_block);
    return raw_ptr;
}

void ControlFlowGraph::add_edge(BasicBlock* from, BasicBlock* to) {
    if (from && to) {
        from->add_successor(to);
        to->add_predecessor(from);
    }
}

BasicBlock* ControlFlowGraph::get_block(const std::string& id) const {
    auto it = blocks.find(id);
    if (it != blocks.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<BasicBlock*> ControlFlowGraph::get_blocks_in_rpo() const {
    std::vector<BasicBlock*> post_order;
    std::unordered_set<BasicBlock*> visited;

    // Recursive DFS helper
    std::function<void(BasicBlock*)> dfs = [&](BasicBlock* block) {
        if (!block || visited.count(block)) return;
        visited.insert(block);
        for (BasicBlock* succ : block->successors) {
            dfs(succ);
        }
        post_order.push_back(block);
    };

    dfs(entry_block);
    std::reverse(post_order.begin(), post_order.end());
    return post_order;
}

void ControlFlowGraph::print_cfg() const {
    std::cout << "\nCFG for function: " << function_name << "\n";
    std::cout << "----------------------------------------\n";

    if (entry_block) {
        std::cout << "Entry Block: " << entry_block->id << "\n";
    }
    if (exit_block) {
        std::cout << "Exit Block: " << exit_block->id << " (conceptual)\n";
    }

    for (const auto& pair : blocks) {
        const BasicBlock* bb = pair.second.get();
        std::cout << "\nBlock ID: " << bb->id;
        if (bb->is_entry) std::cout << " (Entry)";
        if (bb->is_exit) std::cout << " (Exit)";
        if (!bb->label_name.empty()) std::cout << " (Label: " << bb->label_name << ")";
        std::cout << "\n";

        std::cout << "  Statements:\n";
        if (bb->statements.empty()) {
            std::cout << "    (empty)\n";
        } else {
            for (const auto& stmt : bb->statements) {
                std::cout << "    - " << to_string(stmt->getType()) << "\n"; // Use the helper function
            }
        }

        std::cout << "  Successors: ";
        if (bb->successors.empty()) {
            std::cout << "(none)\n";
        } else {
            for (size_t i = 0; i < bb->successors.size(); ++i) {
                std::cout << bb->successors[i]->id << (i == bb->successors.size() - 1 ? "" : ", ");
            }
            std::cout << "\n";
        }

        std::cout << "  Predecessors: ";
        if (bb->predecessors.empty()) {
            std::cout << "(none)\n";
        } else {
            for (size_t i = 0; i < bb->predecessors.size(); ++i) {
                std::cout << bb->predecessors[i]->id << (i == bb->predecessors.size() - 1 ? "" : ", ");
            }
            std::cout << "\n";
        }
    }
    std::cout << "----------------------------------------\n";
}