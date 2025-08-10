#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "RegisterManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(DefaultStatement& node) {
    debug_print("Visiting DefaultStatement node.");
    auto& register_manager = RegisterManager::getInstance();
    // DefaultStatement is handled as part of SwitchonStatement.
    // Its `command` is visited if no preceding cases match.
    // No direct code generation here.
}