#ifndef CONSTANT_FOLDING_PASS_H
#define CONSTANT_FOLDING_PASS_H

#include "Optimizer.h"
#include "AST.h"
#include <string>
#include <memory>

// The ConstantFoldingPass performs compile-time evaluation of constant expressions,
// algebraic simplifications, strength reduction, and constant condition elimination.
class ConstantFoldingPass : public Optimizer {
public:
    ConstantFoldingPass(std::unordered_map<std::string, int64_t>& manifests);
    virtual ~ConstantFoldingPass() = default;

    std::string getName() const override { return "Constant Folding Pass"; }

    // Override the main expression traversal method to intercept and fold expressions.

    // The apply method will initiate the traversal and return the optimized AST.
    ProgramPtr apply(ProgramPtr program) override;

    // --- Specific ASTVisitor overrides for nodes that can be folded ---
    void visit(BinaryOp& node) override;
    void visit(UnaryOp& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(NumberLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(VariableAccess& node) override; // NEW
    void visit(AssignmentStatement& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;

    // --- Added for control flow constant folding and dead branch elimination ---
    void visit(IfStatement& node) override;
    void visit(TestStatement& node) override;
    void visit(WhileStatement& node) override;

    // Default traversal for other nodes
    void visit(ValofExpression& node) override;
    void visit(FloatValofExpression& node) override;
    void visit(VecAllocationExpression& node) override;
    void visit(StringAllocationExpression& node) override;
    void visit(TableExpression& node) override;
    void visit(ForStatement& node) override;
    void visit(RepeatStatement& node) override;
    void visit(BrkStatement& node) override;
    void visit(GlobalVariableDeclaration& node) override;

private:
    // Add this map to track variables known to be constant within the current scope.
    std::unordered_map<std::string, int64_t> known_constants_;

    // Helper to evaluate an expression to a constant integer value.
    OptionalValue<int64_t> evaluate_integer_constant(Expression* expr) const;
    
    // Helper to evaluate an expression to a constant floating-point value.
    OptionalValue<double> evaluate_float_constant(Expression* expr) const;

    // Helper to evaluate an expression to a constant boolean value (0 or 1).
    OptionalValue<bool> evaluate_boolean_constant(Expression* expr) const;

    // Helper to determine BCPL truthiness (0 is false, non-zero is true)
    bool is_bcpl_true(Expression* expr) const;
};

#endif // CONSTANT_FOLDING_PASS_H
