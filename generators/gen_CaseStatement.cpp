#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "RegisterManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(CaseStatement& node) {
    debug_print("Visiting CaseStatement node (Constant: " + std::to_string(static_cast<NumberLiteral*>(node.constant_expr.get())->int_value) + ").");
    auto& register_manager = RegisterManager::getInstance();
    // CaseStatement is handled as part of SwitchonStatement.
    // Its `command` is visited when its condition matches.
    // No direct code generation here.
}