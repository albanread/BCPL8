#include "NewCodeGenerator.h"
#include "AST.h"

void NewCodeGenerator::visit(ConditionalBranchStatement& node) {
    // --- START OF FIX ---
    // This visitor is now intentionally left empty.
    // The actual branch instruction is now generated exclusively by the
    // generate_block_epilogue function, which correctly interprets the
    // block's successors from the CFG. This avoids generating redundant
    // and conflicting branch instructions.
    debug_print("Visiting ConditionalBranchStatement (codegen is handled by block epilogue).");
    // --- END OF FIX ---
}
