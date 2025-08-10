#ifndef AST_H
#define AST_H

#include <vector>
#include <string>
#include <memory>
#include <cstdint>
#include <iostream>

// Custom nullable type since std::optional might not be available
template<typename T>
class OptionalValue {
    bool has_value_;
    T value_;
public:
    OptionalValue() : has_value_(false) {}
    OptionalValue(T value) : has_value_(true), value_(value) {}
    bool has_value() const { return has_value_; }
    T value() const { return value_; }
    void reset() { has_value_ = false; }
};

// Include DataTypes.h for ExprPtr definition
#include "DataTypes.h"

// Smart pointer typedefs for AST nodes
typedef std::unique_ptr<class ASTNode> ASTNodePtr;
typedef std::unique_ptr<class Program> ProgramPtr;
typedef std::unique_ptr<class Declaration> DeclPtr;
// ExprPtr is already defined in DataTypes.h
typedef std::unique_ptr<class Statement> StmtPtr;

// Forward declarations for smart pointers
class ASTNode;
class Program;
class Declaration;
class Expression;
class Statement;
class LabelTargetStatement;
class ConditionalBranchStatement;
class CaseStatement;
class DefaultStatement;
class TableExpression;

// Forward declaration of the ASTVisitor interface
class ASTVisitor;

// Helper function to clone a single unique_ptr (defined in AST.cpp)
template <typename T>
std::unique_ptr<T> clone_unique_ptr(const std::unique_ptr<T>& original_ptr);

// Base class for all AST nodes
class ASTNode {
public:
    enum class NodeType {
        Program, Declaration, Expression, Statement,
        LetDecl, ManifestDecl, StaticDecl, GlobalDecl, FunctionDecl, RoutineDecl, LabelDecl,
        NumberLit, StringLit, CharLit, BooleanLit, VariableAccessExpr, BinaryOpExpr, UnaryOpExpr,
        VectorAccessExpr, CharIndirectionExpr, FloatVectorIndirectionExpr, FunctionCallExpr, SysCallExpr,
        ConditionalExpr, ValofExpr, FloatValofExpr, VecAllocationExpr, StringAllocationExpr, TableExpr,
        AssignmentStmt, RoutineCallStmt, IfStmt, UnlessStmt, TestStmt, WhileStmt, UntilStmt,
        RepeatStmt, ForStmt, SwitchonStmt, GotoStmt, ReturnStmt, FinishStmt, BreakStmt,
        LoopStmt, EndcaseStmt, ResultisStmt, CompoundStmt, BlockStmt, StringStmt, FreeStmt,
        CaseStmt, DefaultStmt, BrkStatement, LabelTargetStmt, ConditionalBranchStmt
    };

    ASTNode(NodeType type) : type_(type) {}
    virtual ~ASTNode() = default;

    virtual NodeType getType() const { return type_; }

    virtual void accept(ASTVisitor& visitor) = 0;
    virtual ASTNodePtr clone() const = 0;
    virtual void replace_with(ASTNodePtr new_node) {
        // Default implementation does nothing.
        // This should be overridden by nodes that can be replaced.
    }

private:
    NodeType type_;
};

// --- Program --- //
class Program : public ASTNode {
public:
    std::vector<DeclPtr> declarations;
    std::vector<StmtPtr> statements;

    Program() : ASTNode(NodeType::Program) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

// --- Declarations --- //
class Declaration : public ASTNode {
public:
    Declaration(NodeType type) : ASTNode(type) {}
    virtual ~Declaration() = default; // Make Declaration polymorphic
};

// --- Statements --- //
class Statement : public ASTNode {
public:
    Statement(NodeType type) : ASTNode(type) {}
    virtual void accept(ASTVisitor& visitor) = 0;
    virtual ASTNodePtr clone() const = 0;

    // --- Live interval/liveness analysis helpers ---
    virtual std::vector<std::string> get_used_variables() const { return {}; }
    virtual std::vector<std::string> get_defined_variables() const { return {}; }
};

class LetDeclaration : public Statement {
public:
    std::vector<std::string> names;
    std::vector<ExprPtr> initializers;
    bool is_float_declaration = false;  // Flag to indicate if this was declared with FLET
    LetDeclaration(std::vector<std::string> names, std::vector<ExprPtr> initializers)
        : Statement(NodeType::LetDecl), names(std::move(names)), initializers(std::move(initializers)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class ManifestDeclaration : public Declaration {
public:
    std::string name;
    int64_t value;

    ManifestDeclaration(std::string name, int64_t value)
        : Declaration(NodeType::ManifestDecl), name(std::move(name)), value(value) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class StaticDeclaration : public Declaration {
public:
    std::string name;
    ExprPtr initializer;

    StaticDeclaration(std::string name, ExprPtr initializer)
        : Declaration(NodeType::StaticDecl), name(std::move(name)), initializer(std::move(initializer)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};



class GlobalDeclaration : public Declaration {
public:
    std::vector<std::pair<std::string, int>> globals;

    GlobalDeclaration(std::vector<std::pair<std::string, int>> globals)
        : Declaration(NodeType::GlobalDecl), globals(std::move(globals)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

// --- New: GlobalVariableDeclaration for top-level LET/FLET --- //
class GlobalVariableDeclaration : public Declaration {
public:
    std::vector<std::string> names;
    std::vector<ExprPtr> initializers;
    bool is_float_declaration = false;

    GlobalVariableDeclaration(std::vector<std::string> names, std::vector<ExprPtr> initializers)
        : Declaration(NodeType::GlobalDecl), names(std::move(names)), initializers(std::move(initializers)) {}

    void accept(ASTVisitor& visitor) override; // To be implemented in AST.cpp
    ASTNodePtr clone() const override;         // To be implemented in AST.cpp
};

class FunctionDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::string> parameters;
    ExprPtr body;

    FunctionDeclaration(std::string name, std::vector<std::string> parameters, ExprPtr body)
        : Declaration(NodeType::FunctionDecl), name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class RoutineDeclaration : public Declaration {
public:
    std::string name;
    std::vector<std::string> parameters;
    StmtPtr body;

    RoutineDeclaration(std::string name, std::vector<std::string> parameters, StmtPtr body)
        : Declaration(NodeType::RoutineDecl), name(std::move(name)), parameters(std::move(parameters)), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class LabelDeclaration : public Declaration {
public:
    std::string name;
    StmtPtr command;

    LabelDeclaration(std::string name, StmtPtr command)
        : Declaration(NodeType::LabelDecl), name(std::move(name)), command(std::move(command)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};



// --- Expressions --- //
class Expression : public ASTNode {
public:
    Expression(NodeType type) : ASTNode(type) {}
    virtual bool is_literal() const { return false; }
};

class NumberLiteral : public Expression {
public:
    enum class LiteralType { Integer, Float };
    LiteralType literal_type;
    int64_t int_value;
    double float_value;

    NumberLiteral(int64_t value) : Expression(NodeType::NumberLit), literal_type(LiteralType::Integer), int_value(value), float_value(0.0) {}
    NumberLiteral(double value) : Expression(NodeType::NumberLit), literal_type(LiteralType::Float), int_value(0), float_value(value) {}

    bool is_literal() const override { return true; }
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class StringLiteral : public Expression {
public:
    std::string value;
    StringLiteral(std::string value) : Expression(NodeType::StringLit), value(std::move(value)) {}

    bool is_literal() const override { return true; }
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class CharLiteral : public Expression {
public:
    char value;
    CharLiteral(char value) : Expression(NodeType::CharLit), value(value) {}

    bool is_literal() const override { return true; }
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class BooleanLiteral : public Expression {
public:
    bool value;
    BooleanLiteral(bool value) : Expression(NodeType::BooleanLit), value(value) {}

    bool is_literal() const override { return true; }
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class VariableAccess : public Expression {
public:
    std::string name;
    std::string unique_name; // New field for unique name if it's a FOR loop variable
    VariableAccess(std::string name) : Expression(NodeType::VariableAccessExpr), name(std::move(name)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class BinaryOp : public Expression {
public:
    enum class Operator {
        Add, Subtract, Multiply, Divide, Remainder,
        Equal, NotEqual, Less, LessEqual, Greater, GreaterEqual,
        LogicalAnd, LogicalOr, Equivalence, NotEquivalence,
        LeftShift, RightShift,
        FloatAdd, FloatSubtract, FloatMultiply, FloatDivide,
        FloatEqual, FloatNotEqual, FloatLess, FloatLessEqual, FloatGreater, FloatGreaterEqual
    };
    Operator op;
    ExprPtr left;
    ExprPtr right;

    BinaryOp(Operator op, ExprPtr left, ExprPtr right)
        : Expression(NodeType::BinaryOpExpr), op(op), left(std::move(left)), right(std::move(right)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class UnaryOp : public Expression {
public:
    enum class Operator { AddressOf, Indirection, LogicalNot, Negate, FloatConvert };
    Operator op;
    ExprPtr operand;

    UnaryOp(Operator op, ExprPtr operand)
        : Expression(NodeType::UnaryOpExpr), op(op), operand(std::move(operand)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class VectorAccess : public Expression {
public:
    ExprPtr vector_expr;
    ExprPtr index_expr;
    VectorAccess(ExprPtr vector_expr, ExprPtr index_expr)
        : Expression(NodeType::VectorAccessExpr), vector_expr(std::move(vector_expr)), index_expr(std::move(index_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class CharIndirection : public Expression {
public:
    ExprPtr string_expr;
    ExprPtr index_expr;
    CharIndirection(ExprPtr string_expr, ExprPtr index_expr)
        : Expression(NodeType::CharIndirectionExpr), string_expr(std::move(string_expr)), index_expr(std::move(index_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class FloatVectorIndirection : public Expression {
public:
    ExprPtr vector_expr;
    ExprPtr index_expr;
    FloatVectorIndirection(ExprPtr vector_expr, ExprPtr index_expr)
        : Expression(NodeType::FloatVectorIndirectionExpr), vector_expr(std::move(vector_expr)), index_expr(std::move(index_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class FunctionCall : public Expression {
public:
    ExprPtr function_expr;
    std::vector<ExprPtr> arguments;
    FunctionCall(ExprPtr function_expr, std::vector<ExprPtr> arguments)
        : Expression(NodeType::FunctionCallExpr), function_expr(std::move(function_expr)), arguments(std::move(arguments)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class SysCall : public Expression {
public:
    std::string function_name; // Name of the syscall function
    ExprPtr syscall_number; // Expression for the syscall number (e.g., 0x2000001)
    std::vector<ExprPtr> arguments; // A vector for the syscall arguments (e.g., exit code)

    SysCall(std::string function_name, ExprPtr syscall_number, std::vector<ExprPtr> arguments)
        : Expression(NodeType::SysCallExpr),
          function_name(std::move(function_name)),
          syscall_number(std::move(syscall_number)),
          arguments(std::move(arguments)) {}

    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class ConditionalExpression : public Expression {
public:
    ExprPtr condition;
    ExprPtr true_expr;
    ExprPtr false_expr;
    ConditionalExpression(ExprPtr condition, ExprPtr true_expr, ExprPtr false_expr)
        : Expression(NodeType::ConditionalExpr), condition(std::move(condition)), true_expr(std::move(true_expr)), false_expr(std::move(false_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class ValofExpression : public Expression {
public:
    StmtPtr body;
    ValofExpression(StmtPtr body)
        : Expression(NodeType::ValofExpr), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class FloatValofExpression : public Expression {
public:
    StmtPtr body;
    FloatValofExpression(StmtPtr body)
        : Expression(NodeType::FloatValofExpr), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class VecAllocationExpression : public Expression {
public:
    ExprPtr size_expr;
    std::string variable_name; // Name of the variable being allocated
    VecAllocationExpression(ExprPtr size_expr)
        : Expression(NodeType::VecAllocationExpr), size_expr(std::move(size_expr)), variable_name("") {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    // Accessor for the variable name
    const std::string& get_variable_name() const { return variable_name; }
};

class StringAllocationExpression : public Expression {
public:
    ExprPtr size_expr;
    StringAllocationExpression(ExprPtr size_expr)
        : Expression(NodeType::StringAllocationExpr), size_expr(std::move(size_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class TableExpression : public Expression {
public:
    std::vector<ExprPtr> initializers;
    TableExpression(std::vector<ExprPtr> initializers)
        : Expression(NodeType::TableExpr), initializers(std::move(initializers)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};


// Forward declare
class FreeStatement;

// Add the class definition
class FreeStatement : public Statement {
public:
    ExprPtr expression_;
    explicit FreeStatement(ExprPtr expression) : Statement(NodeType::FreeStmt), expression_(std::move(expression)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
    NodeType getType() const override { return NodeType::FreeStmt; }
};



class AssignmentStatement : public Statement {
public:
    std::vector<ExprPtr> lhs;
    std::vector<ExprPtr> rhs;
    AssignmentStatement(std::vector<ExprPtr> lhs, std::vector<ExprPtr> rhs)
        : Statement(NodeType::AssignmentStmt), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    // --- Live interval/liveness analysis helpers ---
    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        // Collect variable names from all RHS expressions
        for (const auto& expr : rhs) {
            // For simplicity, only handle VariableAccess for now
            if (expr && expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
                const auto* var = static_cast<const VariableAccess*>(expr.get());
                used.push_back(var->name);
            }
            // TODO: Recursively handle more complex expressions if needed
        }
        return used;
    }
    std::vector<std::string> get_defined_variables() const override {
        std::vector<std::string> defs;
        // Collect variable names from all LHS expressions
        for (const auto& expr : lhs) {
            if (expr && expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
                const auto* var = static_cast<const VariableAccess*>(expr.get());
                defs.push_back(var->name);
            }
            // TODO: Recursively handle more complex LHS if needed
        }
        return defs;
    }
};

class RoutineCallStatement : public Statement {
public:
    ExprPtr routine_expr;
    std::vector<ExprPtr> arguments;
    RoutineCallStatement(ExprPtr routine_expr, std::vector<ExprPtr> arguments)
        : Statement(NodeType::RoutineCallStmt), routine_expr(std::move(routine_expr)), arguments(std::move(arguments)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        // Routine expression itself might be a variable
        if (routine_expr && routine_expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(routine_expr.get());
            used.push_back(var->name);
        }
        // Arguments
        for (const auto& arg : arguments) {
            if (arg && arg->getType() == ASTNode::NodeType::VariableAccessExpr) {
                const auto* var = static_cast<const VariableAccess*>(arg.get());
                used.push_back(var->name);
            }
        }
        return used;
    }
};

class IfStatement : public Statement {
public:
    ExprPtr condition;
    StmtPtr then_branch;
    IfStatement(ExprPtr condition, StmtPtr then_branch)
        : Statement(NodeType::IfStmt), condition(std::move(condition)), then_branch(std::move(then_branch)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (condition && condition->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(condition.get());
            used.push_back(var->name);
        }
        // Optionally, add variables used in then_branch
        if (then_branch) {
            auto branch_vars = then_branch->get_used_variables();
            used.insert(used.end(), branch_vars.begin(), branch_vars.end());
        }
        return used;
    }
};

class UnlessStatement : public Statement {
public:
    ExprPtr condition;
    StmtPtr then_branch;
    UnlessStatement(ExprPtr condition, StmtPtr then_branch)
        : Statement(NodeType::UnlessStmt), condition(std::move(condition)), then_branch(std::move(then_branch)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (condition && condition->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(condition.get());
            used.push_back(var->name);
        }
        // Optionally, add variables used in then_branch
        if (then_branch) {
            auto branch_vars = then_branch->get_used_variables();
            used.insert(used.end(), branch_vars.begin(), branch_vars.end());
        }
        return used;
    }
};

class TestStatement : public Statement {
public:
    ExprPtr condition;
    StmtPtr then_branch;
    StmtPtr else_branch;
    TestStatement(ExprPtr condition, StmtPtr then_branch, StmtPtr else_branch)
        : Statement(NodeType::TestStmt), condition(std::move(condition)), then_branch(std::move(then_branch)), else_branch(std::move(else_branch)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (condition && condition->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(condition.get());
            used.push_back(var->name);
        }
        if (then_branch) {
            auto branch_vars = then_branch->get_used_variables();
            used.insert(used.end(), branch_vars.begin(), branch_vars.end());
        }
        if (else_branch) {
            auto else_vars = else_branch->get_used_variables();
            used.insert(used.end(), else_vars.begin(), else_vars.end());
        }
        return used;
    }
};

class WhileStatement : public Statement {
public:
    ExprPtr condition;
    StmtPtr body;
    WhileStatement(ExprPtr condition, StmtPtr body)
        : Statement(NodeType::WhileStmt), condition(std::move(condition)), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (condition && condition->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(condition.get());
            used.push_back(var->name);
        }
        if (body) {
            auto body_vars = body->get_used_variables();
            used.insert(used.end(), body_vars.begin(), body_vars.end());
        }
        return used;
    }
};

class UntilStatement : public Statement {
public:
    ExprPtr condition;
    StmtPtr body;
    UntilStatement(ExprPtr condition, StmtPtr body)
        : Statement(NodeType::UntilStmt), condition(std::move(condition)), body(std::move(body)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (condition && condition->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(condition.get());
            used.push_back(var->name);
        }
        if (body) {
            auto body_vars = body->get_used_variables();
            used.insert(used.end(), body_vars.begin(), body_vars.end());
        }
        return used;
    }
};

class RepeatStatement : public Statement {
public:
    enum class LoopType { Repeat, RepeatWhile, RepeatUntil };
    LoopType loop_type;
    StmtPtr body;
    ExprPtr condition;
    RepeatStatement(LoopType type, StmtPtr body, ExprPtr condition = nullptr)
        : Statement(NodeType::RepeatStmt), loop_type(type), body(std::move(body)), condition(std::move(condition)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (body) {
            auto body_vars = body->get_used_variables();
            used.insert(used.end(), body_vars.begin(), body_vars.end());
        }
        if (condition && condition->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(condition.get());
            used.push_back(var->name);
        }
        return used;
    }
};

class ForStatement : public Statement {
public:
    std::string loop_variable;
    std::string unique_loop_variable_name; // New field
    std::string unique_step_variable_name;      // To hold the name for the step value's slot
    std::string unique_end_variable_name;       // To hold the name for the (optional) hoisted end value
    ExprPtr start_expr;
    ExprPtr end_expr;
    ExprPtr step_expr;
    StmtPtr body;

    ForStatement(std::string loop_variable, ExprPtr start_expr, ExprPtr end_expr, StmtPtr body, ExprPtr step_expr = nullptr)
        : Statement(NodeType::ForStmt), loop_variable(std::move(loop_variable)), start_expr(std::move(start_expr)),
          end_expr(std::move(end_expr)), body(std::move(body)), step_expr(std::move(step_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
    
    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (start_expr && start_expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(start_expr.get());
            used.push_back(var->name);
        }
        if (end_expr && end_expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(end_expr.get());
            used.push_back(var->name);
        }
        if (step_expr && step_expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(step_expr.get());
            used.push_back(var->name);
        }
        if (body) {
            auto body_vars = body->get_used_variables();
            used.insert(used.end(), body_vars.begin(), body_vars.end());
        }
        return used;
    }
    
    std::vector<std::string> get_defined_variables() const override {
        // The loop variable is defined by the for statement
        return {loop_variable};
    }
};

#include <optional> // Add this include for std::optional

class CaseStatement : public Statement {
public:
    ExprPtr constant_expr;
    StmtPtr command;
    // NEW: Store the resolved constant value here after analysis
    OptionalValue<int64_t> resolved_constant_value;

    CaseStatement(ExprPtr constant_expr, StmtPtr command)
        : Statement(NodeType::CaseStmt), constant_expr(std::move(constant_expr)), command(std::move(command)), resolved_constant_value() {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (constant_expr && constant_expr->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(constant_expr.get());
            used.push_back(var->name);
        }
        if (command) {
            auto cmd_vars = command->get_used_variables();
            used.insert(used.end(), cmd_vars.begin(), cmd_vars.end());
        }
        return used;
    }
};

class DefaultStatement : public Statement {
public:
    StmtPtr command;
    DefaultStatement(StmtPtr command)
        : Statement(NodeType::DefaultStmt), command(std::move(command)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (command) {
            auto cmd_vars = command->get_used_variables();
            used.insert(used.end(), cmd_vars.begin(), cmd_vars.end());
        }
        return used;
    }
};

class SwitchonStatement : public Statement {
public:
    ExprPtr expression;

    std::vector<std::unique_ptr<CaseStatement>> cases;
    std::unique_ptr<DefaultStatement> default_case;

    SwitchonStatement(ExprPtr expression, std::vector<std::unique_ptr<CaseStatement>> cases, std::unique_ptr<DefaultStatement> default_case = nullptr)
        : Statement(NodeType::SwitchonStmt), expression(std::move(expression)), cases(std::move(cases)), default_case(std::move(default_case)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (expression && expression->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(expression.get());
            used.push_back(var->name);
        }
        for (const auto& case_stmt : cases) {
            auto case_vars = case_stmt->get_used_variables();
            used.insert(used.end(), case_vars.begin(), case_vars.end());
        }
        if (default_case) {
            auto default_vars = default_case->get_used_variables();
            used.insert(used.end(), default_vars.begin(), default_vars.end());
        }
        return used;
    }
};

class GotoStatement : public Statement {
public:
    ExprPtr label_expr;
    GotoStatement(ExprPtr label_expr) : Statement(NodeType::GotoStmt), label_expr(std::move(label_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class ReturnStatement : public Statement {
public:
    ReturnStatement() : Statement(NodeType::ReturnStmt) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class FinishStatement : public Statement {
public:
    ExprPtr syscall_number; // Expression for the syscall number (e.g., 0x2000001)
    std::vector<ExprPtr> arguments; // A vector for the syscall arguments (e.g., exit code)

    FinishStatement()
        : Statement(NodeType::FinishStmt),
          syscall_number(nullptr),
          arguments() {}

    FinishStatement(ExprPtr syscall_number, std::vector<ExprPtr> arguments)
        : Statement(NodeType::FinishStmt),
          syscall_number(std::move(syscall_number)),
          arguments(std::move(arguments)) {}

    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class BreakStatement : public Statement {
public:
    BreakStatement() : Statement(NodeType::BreakStmt) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class BrkStatement : public Statement {
public:
    BrkStatement() : Statement(NodeType::BrkStatement) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class LoopStatement : public Statement {
public:
    LoopStatement() : Statement(NodeType::LoopStmt) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class EndcaseStatement : public Statement {
public:
    EndcaseStatement() : Statement(NodeType::EndcaseStmt) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class ResultisStatement : public Statement {
public:
    ExprPtr expression;
    ResultisStatement(ExprPtr expression) : Statement(NodeType::ResultisStmt), expression(std::move(expression)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        if (expression && expression->getType() == ASTNode::NodeType::VariableAccessExpr) {
            const auto* var = static_cast<const VariableAccess*>(expression.get());
            used.push_back(var->name);
        }
        return used;
    }
};

class CompoundStatement : public Statement {
public:
    std::vector<StmtPtr> statements;
    CompoundStatement(std::vector<StmtPtr> statements) : Statement(NodeType::CompoundStmt), statements(std::move(statements)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        for (const auto& stmt : statements) {
            auto stmt_vars = stmt->get_used_variables();
            used.insert(used.end(), stmt_vars.begin(), stmt_vars.end());
        }
        return used;
    }

    std::vector<std::string> get_defined_variables() const override {
        std::vector<std::string> defs;
        for (const auto& stmt : statements) {
            auto stmt_defs = stmt->get_defined_variables();
            defs.insert(defs.end(), stmt_defs.begin(), stmt_defs.end());
        }
        return defs;
    }
};

class BlockStatement : public Statement {
public:
    std::vector<DeclPtr> declarations;
    std::vector<StmtPtr> statements;
    BlockStatement(std::vector<DeclPtr> declarations, std::vector<StmtPtr> statements)
        : Statement(NodeType::BlockStmt), declarations(std::move(declarations)), statements(std::move(statements)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;

    std::vector<std::string> get_used_variables() const override {
        std::vector<std::string> used;
        for (const auto& stmt : statements) {
            auto stmt_vars = stmt->get_used_variables();
            used.insert(used.end(), stmt_vars.begin(), stmt_vars.end());
        }
        return used;
    }

    std::vector<std::string> get_defined_variables() const override {
        std::vector<std::string> defs;
        for (const auto& stmt : statements) {
            auto stmt_defs = stmt->get_defined_variables();
            defs.insert(defs.end(), stmt_defs.begin(), stmt_defs.end());
        }
        return defs;
    }
};

class StringStatement : public Statement {
public:
    ExprPtr size_expr;
    StringStatement(ExprPtr size_expr) : Statement(NodeType::StringStmt), size_expr(std::move(size_expr)) {}
    void accept(ASTVisitor& visitor) override;
    ASTNodePtr clone() const override;
};

class LabelTargetStatement : public Statement {
public:
    std::string labelName;

    // Change Statement() to Statement(NodeType::LabelTargetStmt)
    LabelTargetStatement(std::string name)
        : Statement(NodeType::LabelTargetStmt), labelName(std::move(name)) {}


    void accept(ASTVisitor& visitor) override;
    ASTNode::NodeType getType() const override { return ASTNode::NodeType::LabelTargetStmt; }
    ASTNodePtr clone() const override;
};

class ConditionalBranchStatement : public Statement {
public:
    std::string condition;
    std::string targetLabel;
    ExprPtr condition_expr; // Changed from string

    // Updated constructor to take ExprPtr for condition_expr
    ConditionalBranchStatement(std::string cond, std::string label, ExprPtr cond_expr)
        : Statement(NodeType::ConditionalBranchStmt), condition(std::move(cond)), targetLabel(std::move(label)), condition_expr(std::move(cond_expr)) {}

    void accept(ASTVisitor& visitor) override;
    ASTNode::NodeType getType() const override { return ASTNode::NodeType::ConditionalBranchStmt; }
    ASTNodePtr clone() const override;
};

#endif // AST_H
