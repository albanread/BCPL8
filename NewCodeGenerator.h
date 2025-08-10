#ifndef NEW_CODE_GENERATOR_H
#define NEW_CODE_GENERATOR_H

#include "DataTypes.h"
#pragma once
#include "analysis/ASTAnalyzer.h"
#include "analysis/LiveInterval.h"
#include "SymbolTable.h"

#include "AST.h"
#include "ASTVisitor.h"
#include "DataGenerator.h"
#include "Encoder.h"
#include "InstructionStream.h"
#include "CallFrameManager.h"
#include "RegisterManager.h"
#include "CFGBuilderPass.h"
#include <vector>
#include <string>
#include <map>
#include <stack>
#include <memory>

class LabelManager;
class ASTAnalyzer;
class Identifier;



class NewCodeGenerator : public ASTVisitor {
public:
    NewCodeGenerator(InstructionStream& instruction_stream,
                     RegisterManager& register_manager,
                     LabelManager& label_manager,
                     bool debug,
                     int debug_level,
                     DataGenerator& data_generator,
                     unsigned long long text_segment_size,
                     const CFGBuilderPass& cfg_builder,
                     std::unique_ptr<SymbolTable> symbol_table);

    // Main entry point
    void generate_code(Program& program);

    // Public accessors and emitters
    CallFrameManager* get_current_frame_manager() { return current_frame_manager_.get(); }
    void emit(const Instruction& instr);
    void emit(const std::vector<Instruction>& instrs);

    // Symbol table access
    SymbolTable* get_symbol_table() const { return symbol_table_.get(); }
    
    // Helper functions for LetDeclaration
    bool is_function_like_declaration(const ExprPtr& initializer) const;
    void handle_function_like_declaration(const std::string& name, const ExprPtr& initializer);
    void handle_local_variable_declaration(const std::string& name, bool is_float = false);
    void handle_valof_block_variable(const std::string& name);
    bool is_valof_block_variable(const std::string& name) const;
    void handle_global_variable_declaration(const std::string& name, const ExprPtr& initializer);
    void initialize_variable(const std::string& name, const ExprPtr& initializer, bool is_float_declaration = false);

private:
    const CFGBuilderPass& cfg_builder_;
    VarType current_function_return_type_; // Tracks the return type of the current function

    // ASTVisitor Interface Implementations
    void visit(Program& node) override;
    void visit(LetDeclaration& node) override;
    void visit(ManifestDeclaration& node) override;
    void visit(StaticDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;
    void visit(GlobalVariableDeclaration& node) override;
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
    void visit(FloatValofExpression& node) override;
    
    // Short-circuit evaluation methods
    void generate_short_circuit_and(BinaryOp& node);
    void generate_short_circuit_or(BinaryOp& node);
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
    void visit(FreeStatement& node) override;
    void visit(LabelTargetStatement& node) override;
    void visit(ConditionalBranchStatement& node) override;
    void visit(CompoundStatement& node) override;
    void visit(BlockStatement& node) override;
    void visit(StringStatement& node) override;
    void visit(SysCall& node) override;
    void visit(Expression& node);
    void visit(Declaration& node);

    // Assignment helpers
    void handle_variable_assignment(VariableAccess* var_access, const std::string& value_to_store_reg);
    void handle_vector_assignment(VectorAccess* vec_access, const std::string& value_to_store_reg);
    void handle_char_indirection_assignment(CharIndirection* char_indirection, const std::string& value_to_store_reg);
    void handle_float_vector_indirection_assignment(FloatVectorIndirection* float_vec_indirection, const std::string& value_to_store_reg);
    void handle_indirection_assignment(UnaryOp* unary_op, const std::string& value_to_store_reg);

private:
    uint64_t text_segment_size_;
    uint64_t data_segment_base_addr_ = 0;
    InstructionStream& instruction_stream_;
    RegisterManager& register_manager_;
    LabelManager& label_manager_;
    DataGenerator& data_generator_;
    std::unique_ptr<CallFrameManager> current_frame_manager_;

    bool debug_enabled_;
    int debug_level;
    bool x28_is_loaded_in_current_function_;
    
    std::string expression_result_reg_;
    std::string current_function_name_;
    std::string current_scope_name_;
    size_t block_id_counter_ = 0;

    std::map<std::string, int> current_scope_symbols_;
    std::stack<std::map<std::string, int>> scope_stack_;

    // --- Function Epilogue Label ---
    std::string current_function_epilogue_label_;

    // Private Helper Methods
    void generate_function_like_code(const std::string& name, const std::vector<std::string>& parameters, ASTNode& body_node, bool is_function_returning_value);
    void enter_scope();
    void exit_scope();
    void debug_print(const std::string& message) const;
    void debug_print_level(const std::string& message, int level) const;
    bool is_local_variable(const std::string& name) const;
    std::string get_variable_register(const std::string& var_name);
    void store_variable_register(const std::string& var_name, const std::string& reg_to_store);
    bool lookup_symbol(const std::string& name, Symbol& symbol) const;
    void generate_expression_code(Expression& expr);
    void generate_statement_code(Statement& stmt);
    void process_declarations(const std::vector<DeclPtr>& declarations);
    void process_declaration(Declaration& decl);

    // --- Linear Scan Register Allocation ---
    void performLinearScan(const std::string& functionName, const std::vector<LiveInterval>& intervals);
    void update_spill_offsets();
    std::map<std::string, LiveInterval> current_function_allocation_;
    size_t max_int_pressure_ = 0;  // Maximum integer register pressure observed
    size_t max_fp_pressure_ = 0;   // Maximum floating-point register pressure observed
    void generate_float_to_int_truncation(const std::string& dest_x_reg, const std::string& src_d_reg);

    // CFG-driven codegen helpers
    void generate_block_epilogue(BasicBlock* block);
    
    // Symbol table
    std::unique_ptr<SymbolTable> symbol_table_;
};

#endif // NEW_CODE_GENERATOR_H