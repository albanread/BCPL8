#include "../NewCodeGenerator.h"
#include "../AST.h" // Defines FreeStatement and other AST nodes
#include "../RuntimeManager.h"
#include "../Encoder.h"


void NewCodeGenerator::visit(FreeStatement& node) {
    debug_print("Visiting FreeStatement node.");

    // Special case: FREE CELLS statement
    if (auto* var = dynamic_cast<VariableAccess*>(node.list_expr.get())) {
        std::string var_name = var->name;
        std::transform(var_name.begin(), var_name.end(), var_name.begin(), ::toupper);
        if (var_name == "CELLS") {
            debug_print("Generating code to free the global 'cells' list.");
            emit(Encoder::create_branch_with_link("BCPL_FREE_CELLS"));
            return;
        }
    }

    // 1. Evaluate the expression to get its pointer.
    generate_expression_code(*node.list_expr);
    std::string ptr_reg = expression_result_reg_;

    // 2. Move the pointer to X0 for the runtime call.
    if (ptr_reg != "X0") {
        emit(Encoder::create_mov_reg("X0", ptr_reg));
        register_manager_.release_register(ptr_reg);
    }

    // 3. Determine the type and call the appropriate runtime function.
    std::string runtime_func;
    switch (node.list_expr->getType()) {
        case ASTNode::NodeType::ListExpr:
            runtime_func = "BCPL_FREE_LIST";
            break;
        case ASTNode::NodeType::VecAllocationExpr:
        case ASTNode::NodeType::FVecAllocationExpr:
            runtime_func = "FREEVEC";
            break;
        default:
            throw std::runtime_error("FreeStatement: Can only free a list or vector.");
    }

    size_t offset = RuntimeManager::instance().get_function_offset(runtime_func);
    std::string addr_reg = register_manager_.acquire_scratch_reg(*this);

    Instruction ldr_instr = Encoder::create_ldr_imm(addr_reg, "X19", offset);
    ldr_instr.jit_attribute = JITAttribute::JitAddress;
    emit(ldr_instr);

    Instruction blr_instr = Encoder::create_branch_with_link_register(addr_reg);
    blr_instr.jit_attribute = JITAttribute::JitCall;
    blr_instr.target_label = runtime_func;
    emit(blr_instr);

    register_manager_.release_register(addr_reg);
}


