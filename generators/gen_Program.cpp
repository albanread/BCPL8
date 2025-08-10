#include "NewCodeGenerator.h"
#include "AST.h"
#include <algorithm>
#include <string>

void NewCodeGenerator::visit(Program& node) {
    debug_print("Visiting Program node.");

    // Separate declarations into functions/routines and other types.
    std::vector<Declaration*> function_decls;
    std::vector<Declaration*> other_decls;

    for (const auto& decl : node.declarations) {
        if (dynamic_cast<FunctionDeclaration*>(decl.get()) || dynamic_cast<RoutineDeclaration*>(decl.get())) {
            function_decls.push_back(decl.get());
        } else {
            other_decls.push_back(decl.get());
        }
    }

    // --- JIT FIX: Move the 'START' routine to the end of the processing queue ---
    // The JIT requires that functions be defined before they are called. By ensuring
    // 'START' is generated last, we guarantee all other functions are defined first.
    auto start_it = std::find_if(function_decls.begin(), function_decls.end(), [](Declaration* d) {
        if (auto* r = dynamic_cast<RoutineDeclaration*>(d)) {
            return r->name == "START";
        }
        return false;
    });

    if (start_it != function_decls.end()) {
        debug_print("Moving 'START' routine to the end of the generation queue for JIT compatibility.");
        std::rotate(start_it, start_it + 1, function_decls.end());
    }

    // --- STEP 1: Register all global data declarations ---
    debug_print("Code Generator: Registering global data declarations.");
    for (auto* decl : other_decls) {
        process_declaration(*decl);
    }

    // --- STEP 2: Calculate data offsets NOW ---
    // At this point, DataGenerator knows about all global variables and their initializers.
    data_generator_.calculate_global_offsets();
    debug_print("Code Generator: Global data offsets calculated.");

    // --- STEP 3: Now generate code for functions and routines ---
    debug_print("Code Generator: Generating code for functions and routines.");
    for (auto* decl : function_decls) {
        process_declaration(*decl);
    }

    debug_print("Finished visiting Program node.");
}
