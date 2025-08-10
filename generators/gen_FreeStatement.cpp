#include "../NewCodeGenerator.h"
#include "../AST.h" // Defines FreeStatement and other AST nodes
#include "../RuntimeManager.h"
#include "../Encoder.h"
#include <stdexcept>
void NewCodeGenerator::visit(FreeStatement& node) {
    debug_print("Visiting FreeStatement node.");

    // 1. Evaluate the expression to get the pointer address.
    generate_expression_code(*node.expression_);
    std::string ptr_reg = expression_result_reg_;

    // 2. The ARM64 ABI requires the first argument to be in X0.
    if (ptr_reg != "X0") {
        emit(Encoder::create_mov_reg("X0", ptr_reg));
        register_manager_.release_register(ptr_reg);
    }

    // 3. Generate a call to the "FREEVEC" runtime function.
    std::string func_name = "FREEVEC";
    std::string func_addr_reg = register_manager_.get_cached_routine_reg(func_name);

    if (func_addr_reg.empty()) {
        // Cache MISS: Load the absolute address into a cache register.
        func_addr_reg = register_manager_.get_reg_for_cache_eviction(func_name);

        // --- THE FIX IS HERE ---
        // Look up the real address from RuntimeManager instead of passing 0.
        void* abs_addr_ptr = RuntimeManager::instance().get_function(func_name).address;
        if (!abs_addr_ptr) {
            throw std::runtime_error("Error: Runtime function 'FREEVEC' registered but address is null.");
        }
        uint64_t abs_addr = reinterpret_cast<uint64_t>(abs_addr_ptr);
        auto mov_instructions = Encoder::create_movz_movk_jit_addr(func_addr_reg, abs_addr, func_name);

        // Tag the MOVZ/MOVK sequence with the JitAddress attribute.
        for (auto& instr : mov_instructions) {
            instr.jit_attribute = JITAttribute::JitAddress;
        }

        emit(mov_instructions);
        // --- END OF FIX ---

        
    }

    Instruction blr_instr = Encoder::create_branch_with_link_register(func_addr_reg);
    blr_instr.jit_attribute = JITAttribute::JitCall;
    blr_instr.target_label = func_name; // This is crucial for the AssemblyWriter
    emit(blr_instr);
}
