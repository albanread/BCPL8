#pragma once

#include "../AST.h"
#include "../ASTVisitor.h"
#include <unordered_map>
#include <string>
#include <memory> // For unique_ptr
#include <memory>
#include <stack>
#include <optional>
#include <sstream>

// This pass resolves MANIFEST declarations and replaces VariableAccess nodes
// that refer to them with NumberLiteral nodes.
class ManifestResolutionPass : public ASTVisitor {
public:
    // Constructor. Takes a reference to the global manifests map that it will populate.
    ManifestResolutionPass(std::unordered_map<std::string, int64_t>& global_manifests);

    // The main entry point for applying the pass.
    // Takes ownership of the program AST and returns a potentially modified one.
    ProgramPtr apply(ProgramPtr program);

    // --- ASTVisitor overrides ---
    // Declarations where manifests can be defined.
    void visit(Program& node) override;
    void visit(ManifestDeclaration& node) override;
    void visit(LetDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(BlockStatement& node) override;
    void visit(FinishStatement& node) override;

    // Expressions where variable accesses can occur.
    void visit(VariableAccess& node) override;
    void visit(BinaryOp& node) override;
    void visit(UnaryOp& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(FunctionCall& node) override;
    void visit(VectorAccess& node) override;
    void visit(CharIndirection& node) override;
    void visit(FloatVectorIndirection& node) override;
    void visit(ValofExpression& node) override;
    void visit(FloatValofExpression& node) override {}
    void visit(VecAllocationExpression& node) override;
    void visit(StringAllocationExpression& node) override;
    void visit(TableExpression& node) override;

    // Statements that contain expressions or other statements.
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
    void visit(ResultisStatement& node) override;
    void visit(GlobalVariableDeclaration& node) override;
    void visit(CompoundStatement& node) override;
    void visit(StringStatement& node) override;
    void visit(SysCall& node) override;

    // Default traversal for other nodes, or nodes that don't need specific handling.
    void visit(NumberLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(CharLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(StaticDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;
    void visit(LabelDeclaration& node) override;
    void visit(FreeStatement& node) override;
    void visit(LabelTargetStatement& node) override;
    void visit(ConditionalBranchStatement& node) override;
    void visit(BreakStatement& node) override;
    void visit(LoopStatement& node) override;
    void visit(EndcaseStatement& node) override;
    void visit(BrkStatement& node) override;

protected:
    // Helper to traverse and potentially replace expressions.
    ExprPtr visit_expr(ExprPtr expr);
    // Helper to traverse and potentially replace statements.
    StmtPtr visit_stmt(StmtPtr stmt);
    // Helper to traverse and potentially replace declarations.
    DeclPtr visit_decl(DeclPtr decl);

private:
    std::unordered_map<std::string, int64_t>& global_manifests_; // The map to populate and check
    std::stack<std::unordered_map<std::string, int64_t>> current_scope_manifests_stack_; // For nested scopes
    std::string current_scope_name_; // To track the current named scope (function/routine/block)

    // This member will temporarily hold the transformed/folded node
    // when a `visit` method decides to replace the current node.
    ASTNodePtr current_transformed_node_;

    // Helper to get manifest value from current scope or global
    std::optional<int64_t> get_manifest_value(const std::string& name) const;
};
