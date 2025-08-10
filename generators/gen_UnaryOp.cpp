#include "NewCodeGenerator.h"
#include "LabelManager.h"
#include "analysis/ASTAnalyzer.h"
#include "RuntimeManager.h"
#include <iostream>
#include <stdexcept>

void NewCodeGenerator::visit(UnaryOp& node) {
    debug_print("Visiting UnaryOp node.");
    
    if (node.op == UnaryOp::Operator::FloatConvert && RuntimeManager::instance().isTracingEnabled()) {
        std::cout << "DEBUG: Processing FLOAT conversion operation" << std::endl;
    }

    generate_expression_code(*node.operand);
    std::string operand_reg = expression_result_reg_; // Register holding the operand's result

    auto& register_manager = register_manager_;
    std::string dest_reg;
    if (register_manager.is_scratch_register(operand_reg)) {
        dest_reg = operand_reg;
    } else {
        dest_reg = register_manager.get_free_register();
        emit(Encoder::create_mov_reg(dest_reg, operand_reg));
    }

    switch (node.op) {
        case UnaryOp::Operator::AddressOf:
            if (VariableAccess* var_access = dynamic_cast<VariableAccess*>(node.operand.get())) {
                if (current_frame_manager_ && current_frame_manager_->has_local(var_access->name)) {
                    int offset = current_frame_manager_->get_offset(var_access->name);
                    emit(Encoder::create_add_imm(dest_reg, "X29", offset));
                    debug_print("Generated ADDRESSOF for local variable '" + var_access->name + "' into " + dest_reg + ".");
                } else {
                    std::string var_label = label_manager_.get_label(var_access->name);
                    if (var_label.empty()) {
                        throw std::runtime_error("Cannot get address of variable '" + var_access->name + "': not found.");
                    }
                    emit(Encoder::create_adrp(dest_reg, var_label));
                    emit(Encoder::create_add_literal(dest_reg, dest_reg, var_label));
                    debug_print("Generated ADDRESSOF for global/static variable '" + var_access->name + "' into " + dest_reg + ".");
                }
            } else {
                throw std::runtime_error("AddressOf operator (!expr) can only be applied to variables in BCPL.");
            }
            break;
        case UnaryOp::Operator::Indirection:
            emit(Encoder::create_ldr_imm(dest_reg, operand_reg, 0));
            debug_print("Generated INDIRECTION (load from address in " + operand_reg + ") into " + dest_reg + ".");
            break;
        case UnaryOp::Operator::LogicalNot:
            emit(Encoder::create_cmp_reg(dest_reg, "XZR"));
            emit(Encoder::create_cset_eq(dest_reg));
            emit(Encoder::create_sub_reg(dest_reg, "XZR", dest_reg));
            debug_print("Generated LOGICAL NOT instructions for " + operand_reg + " into " + dest_reg + ".");
            break;
        case UnaryOp::Operator::Negate:
            emit(Encoder::create_sub_reg(dest_reg, "XZR", dest_reg));
            debug_print("Generated NEGATE instruction for " + operand_reg + " into " + dest_reg + ".");
            break;

    // START of new code
    case UnaryOp::Operator::FloatConvert: {
        debug_print("Processing FLOAT conversion operation");
        
        // First check if the operand is already in a floating-point register
        if (register_manager.is_fp_register(operand_reg)) {
            debug_print("Source register " + operand_reg + " is already a floating-point register");
            // No conversion needed if already a float register
            expression_result_reg_ = operand_reg;
        } else {
            // Need to convert integer to float - get a proper FP register
            std::string dest_d_reg = register_manager.acquire_fp_scratch_reg();
            debug_print("Acquired FP destination register: " + dest_d_reg);
            
            // SCVTF <Dd>, <Xn> (Signed Convert to Float)
            emit(Encoder::create_scvtf_reg(dest_d_reg, operand_reg));
            debug_print("Generated SCVTF to convert " + operand_reg + " to " + dest_d_reg);
            
            register_manager.release_register(operand_reg); // Release the source X register
            expression_result_reg_ = dest_d_reg; // The result is now in a D register
        }
        
        // Skip the default destination register handling
        debug_print("Finished FLOAT conversion with result in " + expression_result_reg_);
        return;
    }
    }

    // If the destination register holds a variable, its value has now changed.
    if (register_manager_.registers.count(dest_reg) && register_manager_.registers.at(dest_reg).status == RegisterManager::IN_USE_VARIABLE) {
        register_manager_.mark_dirty(dest_reg);
        debug_print("Marked register " + dest_reg + " as dirty after UnaryOp.");
    }

    // Release operand_reg only after parent operation, and only if it's a scratch and not reused as dest_reg
    if (register_manager.is_scratch_register(operand_reg) && operand_reg != dest_reg) {
        register_manager.release_register(operand_reg);
    }

    expression_result_reg_ = dest_reg;
    debug_print("Finished visiting UnaryOp node.");
}