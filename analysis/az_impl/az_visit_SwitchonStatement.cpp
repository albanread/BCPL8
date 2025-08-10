#include "../ASTAnalyzer.h"
#include "../../AST.h"
#include <iostream>

// Helper to print expression type for diagnostics
static const char* get_expression_type_name(Expression* expr) {
    if (!expr) return "NULL";
    if (dynamic_cast<NumberLiteral*>(expr)) return "NumberLiteral";
    if (dynamic_cast<VariableAccess*>(expr)) return "VariableAccess";
    if (dynamic_cast<BinaryOp*>(expr)) return "BinaryOp";
    if (dynamic_cast<UnaryOp*>(expr)) return "UnaryOp";
    if (dynamic_cast<VectorAccess*>(expr)) return "VectorAccess";
    if (dynamic_cast<CharIndirection*>(expr)) return "CharIndirection";
    if (dynamic_cast<FloatVectorIndirection*>(expr)) return "FloatVectorIndirection";
    if (dynamic_cast<FunctionCall*>(expr)) return "FunctionCall";
    // Add more cases as needed for other expression types
    return "UnknownExpr";
}

void ASTAnalyzer::visit(SwitchonStatement& node) {
    if (trace_enabled_) std::cout << "[ANALYZER TRACE] Visiting SwitchonStatement." << std::endl;

    // Visit the expression being switched on (this expression does NOT need to be a constant)
    if (node.expression) {
        node.expression->accept(*this);
    }

    // Iterate through case statements and enforce constant values
    for (const auto& case_stmt : node.cases) {
        if (!case_stmt || !case_stmt->constant_expr) {
            // This indicates a malformed AST, likely from a parser error.
            std::cerr << "[ERROR] ASTAnalyzer: Case statement with null constant expression encountered. Skipping." << std::endl;
            continue;
        }

        // --- ENFORCE CONSTANT EXPRESSION FOR CASE ---
        bool has_value;
        int64_t constant_value = evaluate_constant_expression(case_stmt->constant_expr.get(), &has_value);

        if (!has_value) {
            // If evaluate_constant_expression sets has_value to false, it's not a valid constant.
            std::cerr << "[ERROR] Semantic Error: CASE constant expression is not a compile-time integer constant."
                      << " (Expression type: " << get_expression_type_name(case_stmt->constant_expr.get()) << ")" << std::endl;
            // In a production compiler, you would likely set a fatal error flag
            // or throw an exception here to halt compilation.
        } else {
            // Store the resolved constant value in the AST node
            case_stmt->resolved_constant_value = constant_value;
            if (trace_enabled_) {
                std::cout << "[ANALYZER TRACE]   Case constant '"
                          << get_expression_type_name(case_stmt->constant_expr.get())
                          << "' resolved to: " << constant_value << ". Stored in AST." << std::endl;
            }
        }

        // Continue visiting the command within the case
        if (case_stmt->command) {
            case_stmt->command->accept(*this);
        }
    }

    // Visit the default case if it exists
    if (node.default_case && node.default_case->command) {
        node.default_case->command->accept(*this);
    }
}