#include "../NewCodeGenerator.h"
#include "../AST.h"
#include <stdexcept>

// Helper function to clone a unique_ptr, assuming it's available from AST_Cloner.cpp
template <typename T>
std::unique_ptr<T> clone_unique_ptr(const std::unique_ptr<T>& original_ptr) {
    if (original_ptr) {
        return std::unique_ptr<T>(static_cast<T*>(original_ptr->clone().release()));
    }
    return nullptr;
}

void NewCodeGenerator::visit(LetDeclaration& node) {
    debug_print("Visiting LetDeclaration node (transforming to Assignment).");

    // This visitor handles LET statements created dynamically by compiler passes
    // like the CFGBuilderPass. It transforms the LET into an AssignmentStatement
    // on the fly and then calls the visitor for AssignmentStatement.

    if (node.names.size() != node.initializers.size()) {
        throw std::runtime_error("LetDeclaration: Mismatch in number of names and initializers.");
    }

    std::vector<ExprPtr> lhs_vec;
    std::vector<ExprPtr> rhs_vec;

    for (const auto& name : node.names) {
        lhs_vec.push_back(std::make_unique<VariableAccess>(name));
    }

    for (const auto& initializer : node.initializers) {
        // We must clone the initializer expression to pass it to the new node.
        rhs_vec.push_back(clone_unique_ptr(initializer));
    }

    // 1. Create the equivalent AssignmentStatement.
    auto assignment_stmt = std::make_unique<AssignmentStatement>(
        std::move(lhs_vec),
        std::move(rhs_vec)
    );

    // 2. Immediately delegate to the correct visitor.
    visit(*assignment_stmt);
}