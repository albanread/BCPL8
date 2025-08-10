#ifndef OPTIMIZER_H
#define OPTIMIZER_H

#include "AST.h"
#include "ASTVisitor.h"
#include <unordered_map>
#include <memory>
#include <algorithm>

// Base class for all optimization passes.
// It implements the ASTVisitor interface and provides default traversal behavior.
// Specific optimization passes will inherit from this class and override
// the visit methods for the AST nodes they intend to optimize.
class Optimizer : public ASTVisitor {
public:
    // Constructor takes a reference to manifests, as some optimizations (like constant folding)
    // might need to access compile-time constants.
    Optimizer(std::unordered_map<std::string, int64_t>& manifests);
    virtual ~Optimizer() = default;

    // The main entry point for applying the optimization pass.
    // This method should be overridden by derived classes to implement their specific logic.
    // It typically initiates the traversal of the AST.
    virtual ProgramPtr apply(ProgramPtr program);

    // Returns the name of the optimization pass.
    virtual std::string getName() const = 0;

    // Handle BRK statement (no-op default)


protected:
    std::unordered_map<std::string, int64_t>& manifests_;

    // Helper method to visit an expression and return a new (potentially optimized) one.
    // Uses current_transformed_node_ for node replacement.
    virtual ExprPtr visit_expr(ExprPtr expr);

    // Helper method to visit a statement and return a new (potentially optimized) one.
    virtual StmtPtr visit_stmt(StmtPtr stmt);

    // Helper method to visit a declaration and return a new (potentially optimized) one.
    virtual DeclPtr visit_decl(DeclPtr decl);

    // --- Default ASTVisitor implementations (traverse children) ---
    // These methods are virtual and can be overridden by derived classes.
    // They now return unique_ptrs to allow for AST transformation.

    // Program
    void visit(Program& node) override;

    // Declarations
    void visit(LetDeclaration& node) override;
    void visit(ManifestDeclaration& node) override;
    void visit(StaticDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(LabelDeclaration& node) override;

    // Expressions
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

    // Statements
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
    void visit(BrkStatement& node) override;
    void visit(FreeStatement& node) override;
    void visit(LabelTargetStatement& node) override;
    void visit(ConditionalBranchStatement& node) override;
    void visit(SysCall& node) override;
    void visit(VecAllocationExpression& node) override;
    void visit(StringAllocationExpression& node) override;
    void visit(TableExpression& node) override;

protected:
    // This member will temporarily hold the transformed/folded node
    // when a `visit` method decides to replace the current node.
    // It's a common pattern for visitors that modify the AST.
    ASTNodePtr current_transformed_node_;
};

#endif // OPTIMIZER_H