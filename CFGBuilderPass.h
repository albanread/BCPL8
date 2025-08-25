#ifndef CFG_BUILDER_PASS_H
#define CFG_BUILDER_PASS_H

#include "AST.h"
#include "ASTVisitor.h"
#include "ControlFlowGraph.h"
#include <optional>
#include <unordered_map>
#include <utility> // For std::pair
#include <string>
#include <vector>
#include <memory>

// The CFGBuilderPass traverses the AST and constructs a Control Flow Graph
// for each function and routine.
class CFGBuilderPass : public ASTVisitor {
public:
    CFGBuilderPass(bool trace_enabled = false);
    std::string getName() const { return "CFG Builder Pass"; }
    void visit(BrkStatement& node) override;
    void visit(GlobalVariableDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;

    // Main entry point to start building CFGs for the entire program.
    void build(Program& program);

    // Returns the map of built CFGs (function_name -> CFG).
    const std::unordered_map<std::string, std::unique_ptr<ControlFlowGraph>>& get_cfgs() const { return function_cfgs; }

private:
    std::unordered_map<std::string, std::unique_ptr<ControlFlowGraph>> function_cfgs; // Stores a CFG for each function/routine
    ControlFlowGraph* current_cfg; // Pointer to the CFG currently being built
    BasicBlock* current_basic_block; // Pointer to the basic block currently being populated
    std::unordered_map<std::string, BasicBlock*> label_targets; // Maps label names to their corresponding basic blocks
    int block_id_counter; // Counter for generating unique basic block IDs within a function

    // Stacks to keep track of control flow targets for BREAK, LOOP, ENDCASE
    std::vector<BasicBlock*> break_targets; 
    std::vector<BasicBlock*> loop_targets; 
    std::vector<BasicBlock*> endcase_targets; 

    // A list to store all GOTO statements and the block they terminate.
    std::vector<std::pair<GotoStatement*, BasicBlock*>> unresolved_gotos_;

    bool trace_enabled_; // Flag to control debug output

    // Helper for printing debug messages
    void debug_print(const std::string& message);

    // Helper to create a new basic block and add it to the current CFG
    BasicBlock* create_new_basic_block(const std::string& id_prefix = "BB_");

    // Helper to end the current basic block and start a new one, adding a fall-through edge
    void end_current_block_and_start_new();
    
    // Resolves all pending GOTO statements by creating edges in the CFG
    void resolve_gotos();

    // ASTVisitor overrides
    void visit(Program& node) override;
    void visit(ForEachStatement& node) override;

    // Helpers for FOREACH/FFOREACH CFG construction
    void build_vector_foreach_cfg(ForEachStatement& node);
    void build_list_foreach_cfg(ForEachStatement& node);
    void visit(LetDeclaration& node) override;
    void visit(ManifestDeclaration& node) override;
    void visit(StaticDeclaration& node) override;

    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(LabelDeclaration& node) override; // Keep to satisfy pure virtual requirement

    void visit(NumberLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(CharLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(VariableAccess& node) override;
    void visit(BinaryOp& node) override;
    void visit(UnaryOp& node) override;
    void visit(VectorAccess& node) override;
    void visit(CharIndirection& node) override;
    void visit(FloatVectorIndirection& node) override;
    void visit(FunctionCall& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(ValofExpression& node) override;


    void visit(AssignmentStatement& node) override;
    void visit(RoutineCallStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(UnlessStatement& node) override;
    void visit(TestStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(UntilStatement& node) override;
    void visit(RepeatStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(SwitchonStatement& node) override;
    void visit(CaseStatement& node) override;
    void visit(DefaultStatement& node) override;
    void visit(GotoStatement& node) override;
    void visit(ReturnStatement& node) override;
    void visit(FinishStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(LoopStatement& node) override;
    void visit(EndcaseStatement& node) override;
    void visit(ResultisStatement& node) override;
    void visit(CompoundStatement& node) override;
    void visit(BlockStatement& node) override;
    void visit(StringStatement& node) override;
    void visit(FreeStatement& node) override;
    void visit(LabelTargetStatement& node) override;
    void visit(ConditionalBranchStatement& node) override;
    void visit(SysCall& node) override;
    void visit(VecAllocationExpression& node) override;
    void visit(StringAllocationExpression& node) override;
    void visit(TableExpression& node) override;

    // FloatValofExpression visitor
    void visit(FloatValofExpression& node) override;
};

#endif // CFG_BUILDER_PASS_H
