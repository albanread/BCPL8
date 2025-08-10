#ifndef DEBUG_PRINTER_H
#define DEBUG_PRINTER_H

#include "AST.h"
#include "ASTVisitor.h"
#include <iostream>
#include <string>



/**
 * @class DebugPrinter
 * @brief An ASTVisitor that prints a human-readable representation of the AST.
 *
 * This class traverses the AST and prints each node's type and value with
 * indentation to clearly show the tree structure. It is an essential tool
 * for debugging the parser and subsequent compiler stages.
 */
class DebugPrinter : public ASTVisitor {
public:
    /**
     * @brief Prints the entire Program AST to standard output.
     * @param program The root Program node of the AST to print.
     */
    void print(Program& program);

    // Visitor overrides for each AST node type
    void visit(Program& node) override;
    void visit(LetDeclaration& node) override;
    void visit(FreeStatement& node) override;


    void visit(ManifestDeclaration& node) override;
    void visit(StaticDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(LabelDeclaration& node) override;
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
    void visit(FloatValofExpression& node) override {}
    void visit(SysCall& node) override;
    void visit(VecAllocationExpression& node) override;
    void visit(StringAllocationExpression& node) override;
    void visit(TableExpression& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(RoutineCallStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(UnlessStatement& node) override;
    void visit(TestStatement& node) override;
    void visit(BrkStatement& node) override;
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
    void visit(LabelTargetStatement& node) override;
    void visit(ConditionalBranchStatement& node) override;
    void visit(GlobalVariableDeclaration& node) override;

private:
    int indent_level_ = 0;
    void print_indent();
    void print_line(const std::string& text);

    // Helper to print a node with increased indentation
    template <typename T>
    void print_child(T& node) {
        indent_level_++;
        if (node) node->accept(*this);
        indent_level_--;
    }

    // Implementation moved to DebugPrinter.cpp
};



#endif // DEBUG_PRINTER_H
