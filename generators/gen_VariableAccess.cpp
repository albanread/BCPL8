#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include "RuntimeManager.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(VariableAccess& node) {
    debug_print("Visiting VariableAccess node for '" + node.name + "'.");

    // Manifest constants are not needed here; removed erroneous call.
    // Manifest lookup is not implemented in this generator.

    // Manifest constant lookup and literal generation removed.

    // 4. If not a manifest constant, proceed with variable lookup
    expression_result_reg_ = get_variable_register(node.name);

    debug_print("Variable '" + node.name + "' value loaded into " + expression_result_reg_);
}