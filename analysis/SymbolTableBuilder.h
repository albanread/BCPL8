#ifndef SYMBOL_TABLE_BUILDER_H
#define SYMBOL_TABLE_BUILDER_H

#include "../AST.h"
#include "../ASTVisitor.h"
#include "../SymbolTable.h"
#include "DataTypes.h"



/**
 * @class SymbolTableBuilder
 * @brief Traverses the AST to build a comprehensive symbol table.
 *
 * This pass runs immediately after parsing and before semantic analysis.
 * It populates a SymbolTable with all declarations in the program, maintaining
 * proper scoping and storing type information.
 */
class SymbolTableBuilder : public ASTVisitor {
public:
    /**
     * @brief Constructor
     * @param enable_tracing Whether to print debug information
     */
    SymbolTableBuilder(bool enable_tracing = false);

    /**
     * @brief Build symbol table for the entire program
     * @param program The AST root
     * @return The populated symbol table
     */
    std::unique_ptr<SymbolTable> build(Program& program);

    // AST Visitor methods
    void visit(Program& node) override;
    void visit(LetDeclaration& node) override;
    void visit(ManifestDeclaration& node) override;
    void visit(StaticDeclaration& node) override;
    void visit(GlobalDeclaration& node) override;
    void visit(GlobalVariableDeclaration& node) override;
    void visit(FunctionDeclaration& node) override;
    void visit(RoutineDeclaration& node) override;
    void visit(LabelDeclaration& node) override;
    void visit(BlockStatement& node) override;
    void visit(ForStatement& node) override;
    void visit(FloatValofExpression& node) override;
    void visit(ValofExpression& node) override;

private:
    // The symbol table being built
    std::unique_ptr<SymbolTable> symbol_table_;

    // Trace flag for debugging
    bool enable_tracing_;

    // Helper methods
    void trace(const std::string& message) const;
    void report_duplicate_symbol(const std::string& name) const;
};



#endif // SYMBOL_TABLE_BUILDER_H