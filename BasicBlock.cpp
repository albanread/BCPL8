#include "BasicBlock.h"

BasicBlock::BasicBlock(std::string id, bool is_entry, bool is_exit, std::string label_name)
    : id(std::move(id)),
      is_entry(is_entry),
      is_exit(is_exit),
      label_name(std::move(label_name)) {}

void BasicBlock::add_statement(StmtPtr stmt) {
    statements.push_back(std::move(stmt));
}

void BasicBlock::add_successor(BasicBlock* successor) {
    if (successor) {
        successors.push_back(successor);
    }
}

void BasicBlock::add_predecessor(BasicBlock* predecessor) {
    if (predecessor) {
        predecessors.push_back(predecessor);
    }
}

bool BasicBlock::ends_with_control_flow() const {
    if (statements.empty()) {
        return false;
    }
    // Check the last statement in the block
    const StmtPtr& last_stmt = statements.back();
    switch (last_stmt->getType()) {
        case ASTNode::NodeType::GotoStmt:
        case ASTNode::NodeType::ReturnStmt:
        case ASTNode::NodeType::FinishStmt:
        case ASTNode::NodeType::BreakStmt:
        case ASTNode::NodeType::LoopStmt:
        case ASTNode::NodeType::EndcaseStmt:
        case ASTNode::NodeType::ConditionalBranchStmt:
        case ASTNode::NodeType::ResultisStmt:
            return true;
        default:
            return false;
    }
}
