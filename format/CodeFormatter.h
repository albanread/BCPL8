#ifndef CODE_FORMATTER_H
#define CODE_FORMATTER_H

#include "ASTVisitor.h"
#include "AST.h"
#include <sstream>
#include <string>
#include <vector>
#include <memory>

class CodeFormatter : public ASTVisitor {
public:
    CodeFormatter();
    std::string format(Program& program);

private:
    // Helper Methods
    void indent();
    void dedent();
    void write(const std::string& text);
    void write_line(const std::string& text = "");
    void write_indent();
    std::string uppercase(std::string s);

    // Expression Formatting
    std::string format_expression(Expression* expr);
    std::string format_binary_op(BinaryOp* op);
    std::string format_unary_op(UnaryOp* op);
    std::string format_function_call(FunctionCall* call);

    // Visitor Overrides
    void visit(Program& node) override;
    void visit(LetDeclaration& node) override;
    void visit(ManifestDeclaration& node) override;
    void visit(StaticDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;
    void visit(GlobalVariableDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(LabelDeclaration& node) override;

    // Expressions (for completeness, but handled in format_expression)
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
    void visit(SysCall& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(ValofExpression& node) override;
    void visit(FloatValofExpression& node) override;
    void visit(VecAllocationExpression& node) override;
    void visit(StringAllocationExpression& node) override;
    void visit(TableExpression& node) override;

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
    void visit(BrkStatement& node) override;
    void visit(LoopStatement& node) override;
    void visit(EndcaseStatement& node) override;
    void visit(ResultisStatement& node) override;
    void visit(CompoundStatement& node) override;
    void visit(BlockStatement& node) override;
    void visit(StringStatement& node) override;
    void visit(FreeStatement& node) override;
    void visit(LabelTargetStatement& node) override;
    void visit(ConditionalBranchStatement& node) override;

    std::stringstream output_;
    int indent_level_ = 0;
};

#endif // CODE_FORMATTER_H