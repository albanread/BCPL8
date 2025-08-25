#pragma once

// --- Forward declarations for all AST node classes ---
// Add every AST node class here, so the visitor interface can reference them
class Program;
class LetDeclaration;
class ManifestDeclaration;
class StaticDeclaration;
class GlobalDeclaration;
class FunctionDeclaration;
class RoutineDeclaration;
class LabelDeclaration;
class NumberLiteral;
class StringLiteral;
class CharLiteral;
class BooleanLiteral;
class VariableAccess;
class BinaryOp;
class UnaryOp;
class VectorAccess;
class CharIndirection;
class FloatVectorIndirection;
class FunctionCall;
class SysCall;
class BitfieldAccess;
class ListExpression;
class AppendExpression;
class ConditionalExpression;
class ValofExpression;
class FloatValofExpression;
class VecAllocationExpression;
class FVecAllocationExpression;
class StringAllocationExpression;
class TableExpression;
class AssignmentStatement;
class RoutineCallStatement;
class IfStatement;
class UnlessStatement;
class TestStatement;
class WhileStatement;
class UntilStatement;
class RepeatStatement;
class ForStatement;
class ForEachStatement;
class SwitchonStatement;
class GotoStatement;
class ReturnStatement;
class FinishStatement;
class BreakStatement;
class LoopStatement;
class EndcaseStatement;
class ResultisStatement;
class CompoundStatement;
class BlockStatement;
class StringStatement;
class FreeStatement;
class CaseStatement;
class DefaultStatement;
class BrkStatement;
class LabelTargetStatement;
class ConditionalBranchStatement;
class GlobalVariableDeclaration;

// --- The base visitor interface ---
class ASTVisitor {
public:
    virtual ~ASTVisitor() = default;

    // --- Add a visit method for every AST node type ---
    virtual void visit(Program& node) {}
    virtual void visit(LetDeclaration& node) {}
    virtual void visit(ManifestDeclaration& node) {}
    virtual void visit(StaticDeclaration& node) {}
    virtual void visit(GlobalDeclaration& node) {}
    virtual void visit(FunctionDeclaration& node) {}
    virtual void visit(RoutineDeclaration& node) {}
    virtual void visit(LabelDeclaration& node) {}
    virtual void visit(NumberLiteral& node) {}
    virtual void visit(StringLiteral& node) {}
    virtual void visit(CharLiteral& node) {}
    virtual void visit(BooleanLiteral& node) {}
    virtual void visit(VariableAccess& node) {}
    virtual void visit(BinaryOp& node) {}
    virtual void visit(UnaryOp& node) {}
    virtual void visit(VectorAccess& node) {}
    virtual void visit(CharIndirection& node) {}
    virtual void visit(FloatVectorIndirection& node) {}
    virtual void visit(FunctionCall& node) {}
    virtual void visit(SysCall& node) {}
    virtual void visit(BitfieldAccess& node) {}
    virtual void visit(ListExpression& node) {}
    virtual void visit(AppendExpression& node) {}
    virtual void visit(ConditionalExpression& node) {}
    virtual void visit(ValofExpression& node) {}
    virtual void visit(FloatValofExpression& node) {}
    virtual void visit(VecAllocationExpression& node) {}
    virtual void visit(FVecAllocationExpression& node) {}
    virtual void visit(StringAllocationExpression& node) {}
    virtual void visit(TableExpression& node) {}
    virtual void visit(AssignmentStatement& node) {}
    virtual void visit(RoutineCallStatement& node) {}
    virtual void visit(IfStatement& node) {}
    virtual void visit(UnlessStatement& node) {}
    virtual void visit(TestStatement& node) {}
    virtual void visit(WhileStatement& node) {}
    virtual void visit(UntilStatement& node) {}
    virtual void visit(RepeatStatement& node) {}
    virtual void visit(ForStatement& node) {}
    virtual void visit(ForEachStatement& node) {}
    virtual void visit(SwitchonStatement& node) {}
    virtual void visit(GotoStatement& node) {}
    virtual void visit(ReturnStatement& node) {}
    virtual void visit(FinishStatement& node) {}
    virtual void visit(BreakStatement& node) {}
    virtual void visit(LoopStatement& node) {}
    virtual void visit(EndcaseStatement& node) {}
    virtual void visit(ResultisStatement& node) {}
    virtual void visit(CompoundStatement& node) {}
    virtual void visit(BlockStatement& node) {}
    virtual void visit(StringStatement& node) {}
    virtual void visit(FreeStatement& node) {}
    virtual void visit(CaseStatement& node) {}
    virtual void visit(DefaultStatement& node) {}
    virtual void visit(BrkStatement& node) {}
    virtual void visit(LabelTargetStatement& node) {}
    virtual void visit(ConditionalBranchStatement& node) {}
    virtual void visit(GlobalVariableDeclaration& node) {}
};