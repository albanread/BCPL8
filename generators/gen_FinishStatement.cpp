#include "NewCodeGenerator.h"
#include "AST.h"
#include <vector>
#include <memory>

void NewCodeGenerator::visit(FinishStatement& node) {
    debug_print("Visiting FinishStatement node.");

    // The FINISH statement is equivalent to an exit(0) syscall.
    // We will construct a SysCall AST node on the fly and then visit it.

    // 1. Define the syscall number for exit() on macOS (0x2000001).
    //    FIX: Use NumberLiteral and cast to int64_t to resolve ambiguity.
    ExprPtr syscall_number = std::make_unique<NumberLiteral>(static_cast<int64_t>(0x2000001));

    // 2. Define the argument for the syscall (the exit code, 0).
    //    FIX: Use NumberLiteral and cast to int64_t.
    ExprPtr exit_code = std::make_unique<NumberLiteral>(static_cast<int64_t>(0));

    // 3. Place the argument(s) into a vector.
    std::vector<ExprPtr> arguments;
    arguments.push_back(std::move(exit_code));

    // 4. Instantiate the SysCall node with the required three arguments.
    //    FIX: Add a string name like "exit" as the first argument.
    SysCall exit_syscall("exit", std::move(syscall_number), std::move(arguments));

    // 5. Visit the SysCall node to trigger its code generation.
    visit(exit_syscall);
}
