#include "ASTAnalyzer.h"
#include <iostream>

void ASTAnalyzer::visit(SysCall& node) {
    if (trace_enabled_) {
        std::cout << "[ANALYZER TRACE] Visiting SysCall: " << node.function_name << std::endl;
        std::cout << "[ANALYZER TRACE] SysCall Number: " << node.syscall_number.get() << std::endl;
        std::cout << "[ANALYZER TRACE] Arguments: " << node.arguments.size() << " provided." << std::endl;
    }

    // Analyze the syscall number
    if (!node.syscall_number) {
        std::cerr << "[ERROR] SysCall has no valid syscall number." << std::endl;
        return;
    }

    // Analyze each argument
    for (const auto& arg : node.arguments) {
        if (!arg) {
            std::cerr << "[ERROR] SysCall contains a null argument." << std::endl;
            continue;
        }
        arg->accept(*this); // Visit each argument
    }
}