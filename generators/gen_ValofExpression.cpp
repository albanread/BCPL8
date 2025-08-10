#include "../NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"



void NewCodeGenerator::visit(ValofExpression& node) {
    debug_print("Visiting ValofExpression node.");
    // `VALOF <stmt> RESULTIS expr`
    // This creates a block that evaluates to a value.

    // Enter a new scope for the VALOF block.
    // A VALOF block is part of the parent function's scope.

    // The result of the ValofExpression will be determined by the `RESULTIS` statement.
    // The `RESULTIS` statement will set `expression_result_reg_`.

    // Variables declared in the VALOF block are already registered by the pre-scan.

    // Traverse and evaluate all statements or expressions in the VALOF body
    if (node.body) {
        generate_statement_code(*node.body);
    }

    // Exit the scope of the VALOF block.
    // Exiting VALOF block processing.

    // `expression_result_reg_` should now contain the value from the `RESULTIS` statement.
    debug_print("Finished visiting ValofExpression node.");
}