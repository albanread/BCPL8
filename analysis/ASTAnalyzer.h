#pragma once

#include "../AST.h"
#include "../ASTVisitor.h"
#include <map>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "../DataTypes.h"
#include "../SymbolTable.h"

// VarType is now defined in DataTypes.h
// Removed custom optional implementation



/**
 * @class ASTAnalyzer
 * @brief Traverses the Abstract Syntax Tree to gather semantic information.
 */
class ASTAnalyzer : public ASTVisitor {
  public:
    // VarType now in global namespace
    const std::map<std::string, VarType>& get_function_return_types() const { return function_return_types_; }

    static ASTAnalyzer& getInstance();
    void analyze(Program& program, SymbolTable* symbol_table = nullptr);
    void transform(Program& program);
    void print_report() const;

    // Get the symbol table
    void visit(ListExpression& node) override;
    SymbolTable* get_symbol_table() const { return symbol_table_; }

    const std::map<std::string, FunctionMetrics>& get_function_metrics() const { return function_metrics_; }
    std::map<std::string, FunctionMetrics>& get_function_metrics_mut() { return function_metrics_; }
    const std::map<std::string, std::string>& get_variable_definitions() const { return variable_definitions_; }
    const ForStatement& get_for_statement(const std::string& unique_name) const;

    // Infer the type of an expression (INTEGER, FLOAT, POINTER, etc.)
    VarType infer_expression_type(const Expression* expr) const;
    bool function_accesses_globals(const std::string& function_name) const;
    int64_t evaluate_constant_expression(Expression* expr, bool* has_value) const;
    bool is_local_routine(const std::string& name) const;
    bool is_local_function(const std::string& name) const;
    bool is_local_float_function(const std::string& name) const;
    VarType get_variable_type(const std::string& function_name, const std::string& var_name) const;
    FunctionType get_runtime_function_type(const std::string& name) const;

    // Visitor methods
    void visit(Program& node) override;
    void visit(LetDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(BlockStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(ForEachStatement& node) override;
    void visit(FunctionCall& node) override;
    void visit(RoutineCallStatement& node) override;
    void visit(VariableAccess& node) override;
    void visit(VecAllocationExpression& node) override;
    void visit(VecInitializerExpression& node) override;
    void visit(SwitchonStatement& node) override;
    void visit(ManifestDeclaration& node) override;
    void visit(StaticDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;
    void visit(LabelDeclaration& node) override;
    void visit(NumberLiteral& node) override;
    void visit(StringLiteral& node) override;
    void visit(CharLiteral& node) override;
    void visit(BooleanLiteral& node) override;
    void visit(BinaryOp& node) override;
    void visit(UnaryOp& node) override;
    void visit(VectorAccess& node) override;
    void visit(CharIndirection& node) override;
    void visit(FloatVectorIndirection& node) override;
    void visit(BitfieldAccessExpression& node) override;
    void visit(ConditionalExpression& node) override;
    void visit(ValofExpression& node) override;
    void visit(StringAllocationExpression& node) override;
    void visit(TableExpression& node) override;
    void visit(AssignmentStatement& node) override;
    void visit(IfStatement& node) override;
    void visit(UnlessStatement& node) override;
    void visit(TestStatement& node) override;
    void visit(WhileStatement& node) override;
    void visit(UntilStatement& node) override;
    void visit(RepeatStatement& node) override;
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
    void visit(StringStatement& node) override;
    void visit(GlobalVariableDeclaration& node) override;
    void visit(BrkStatement& node) override;
    void visit(FreeStatement& node) override;
    void visit(LabelTargetStatement& node) override;
    void visit(ConditionalBranchStatement& node) override;
    void visit(SysCall& node) override;

  private:
    ASTAnalyzer();
    ASTAnalyzer(const ASTAnalyzer&) = delete;
    ASTAnalyzer& operator=(const ASTAnalyzer&) = delete;

    void reset_state();
    std::string get_effective_variable_name(const std::string& original_name) const;
    void first_pass_discover_functions(Program& program);
    void transform_let_declarations(std::vector<DeclPtr>& declarations);

    // --- Scope Management ---
    std::string current_function_scope_;
    std::string current_lexical_scope_;

public:
    // Expose setter and getter for current_function_scope_
    void set_current_function_scope(const std::string& scope) { current_function_scope_ = scope; }
    std::string get_current_function_scope() const { return current_function_scope_; }

    // --- Data Members ---
    std::map<std::string, FunctionMetrics> function_metrics_;
    std::map<std::string, VarType> function_return_types_;
    std::map<std::string, std::string> variable_definitions_;
    std::set<std::string> local_function_names_;
    std::set<std::string> local_routine_names_;
    std::set<std::string> local_float_function_names_;
    int for_loop_var_counter_ = 0;
    std::map<std::string, std::string> for_variable_unique_aliases_;

    // Visitor for FloatValofExpression
    void visit(FloatValofExpression& node) override;
    std::stack<std::map<std::string, std::string>> active_for_loop_scopes_;
    int for_loop_instance_suffix_counter = 0;
    std::map<std::string, const ForStatement*> for_statements_;
    bool trace_enabled_ = false;
    
    // Symbol table reference (owned by the compiler)
    SymbolTable* symbol_table_ = nullptr;
};