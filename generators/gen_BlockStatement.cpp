#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include <iostream>
#include <stdexcept>
#include <sstream>

void NewCodeGenerator::visit(BlockStatement& node) {
    debug_print("Visiting BlockStatement node.");

    // --- NEW: Update and restore scope name ---
    std::string previous_scope = current_scope_name_;
    std::ostringstream block_name_ss;
    block_name_ss << previous_scope << "_block_" << block_id_counter_++;
    current_scope_name_ = block_name_ss.str();
    debug_print("Entering codegen block scope: " + current_scope_name_);
    // --- END NEW ---

    // 1. Enter a new scope.
    enter_scope();

    // 2. Process declarations within this block. These are local to the block.
    // They will be added to the current `CallFrameManager` if within a function,
    // or handled appropriately if at global level (which is unusual for BLOCK).
    for (const auto& decl : node.declarations) {
        if (decl) {
            debug_print_level(
                std::string("BlockStatement: About to process declaration node at ") +
                std::to_string(reinterpret_cast<uintptr_t>(decl.get())),
                4
            );
        }
    }
    process_declarations(node.declarations);

    // 3. Generate code for statements within this block.
    for (const auto& stmt : node.statements) {
        if (stmt) {
            generate_statement_code(*stmt);
        }
    }

    // 4. Exit the scope.
    exit_scope();

    // Note: If a `BLOCK` introduces new locals, and it's inside a function,
    // the `CallFrameManager` would need to support dynamic stack allocation
    // within the function body (e.g., by adjusting SP mid-function).
    // Current `CallFrameManager` assumes all locals are known at prologue time.
    // This is a common design choice for simpler compilers. If dynamic allocation
    // per block is needed, `SUB SP, SP, #size` and `ADD SP, SP, #size` would be inserted.
    // For now, `add_local` on the `CallFrameManager` is only called during the pre-scan.
    // This implies that `LET`s inside `BLOCK` statements *must* be handled during the
    // pre-scan pass of the enclosing `FunctionDeclaration` or `RoutineDeclaration`.
    // If not, this current implementation would lead to errors with `get_offset` or `add_local`.
    debug_print("Finished visiting BlockStatement node.");
}