#include "CallFrameManager.h"
#include "Encoder.h"
#include <vector>
#include <stdexcept>

std::vector<Instruction> CallFrameManager::generate_epilogue() {
    if (!is_prologue_generated) {
        throw std::runtime_error("Cannot generate epilogue before prologue.");
    }

    std::vector<Instruction> epilogue_code;

    // 1. Restore all callee-saved registers that were saved in the prologue.
    for (const auto& reg : callee_saved_registers_to_save) {
        int offset = variable_offsets.at(reg);
        Instruction instr = Encoder::create_ldr_imm(reg, "X29", offset);
        if (reg == "X19" || reg == "X20") {
            instr.assembly_text += " ; [JIT_RESTORE]";
        }
        instr.assembly_text += " ; Restored Reg: " + reg + " @ FP+" + std::to_string(offset);
        epilogue_code.push_back(instr);
    }

    // Only add canary checks if stack canaries are enabled
    if (enable_stack_canaries) {
        // Define fixed canary offsets for checks.
        int upper_canary_offset = 16;
        int lower_canary_offset = 16 + CANARY_SIZE; // Assumes CANARY_SIZE is defined.

        // Canary Check: Upper Canary. Branch to handler on failure.
        epilogue_code.push_back(Encoder::create_ldr_imm("X10", "X29", upper_canary_offset));
        epilogue_code.back().assembly_text += " ; Load Upper Stack Canary for check";
        for (const auto& instr : Encoder::create_movz_movk_abs64("X11", UPPER_CANARY_VALUE, "")) {
            epilogue_code.push_back(instr);
        }
        epilogue_code.back().assembly_text += " ; Load Expected UPPER_CANARY_VALUE";
        epilogue_code.push_back(Encoder::create_cmp_reg("X10", "X11"));
        epilogue_code.back().assembly_text += " ; Compare Upper Canary";
        epilogue_code.push_back(Encoder::create_branch_conditional("NE", function_name + "_stackprot_upper"));
        epilogue_code.back().assembly_text += " ; Branch if Upper Canary Corrupted";

        // Canary Check: Lower Canary. Branch to handler on failure.
        epilogue_code.push_back(Encoder::create_ldr_imm("X10", "X29", lower_canary_offset));
        epilogue_code.back().assembly_text += " ; Load Lower Stack Canary for check";
        for (const auto& instr : Encoder::create_movz_movk_abs64("X11", LOWER_CANARY_VALUE, "")) {
            epilogue_code.push_back(instr);
        }
        epilogue_code.back().assembly_text += " ; Load Expected LOWER_CANARY_VALUE";
        epilogue_code.push_back(Encoder::create_cmp_reg("X10", "X11"));
        epilogue_code.back().assembly_text += " ; Compare Lower Canary";
        epilogue_code.push_back(Encoder::create_branch_conditional("NE", function_name + "_stackprot_lower"));
        epilogue_code.back().assembly_text += " ; Branch if Lower Canary Corrupted";
    }

    // 5. Normal return path: tear down the stack frame.
    epilogue_code.push_back(Encoder::create_mov_sp_fp());
    epilogue_code.back().assembly_text += " ; Deallocate frame by moving FP to SP";
    epilogue_code.push_back(Encoder::create_ldr_imm("X29", "SP", 0));
    epilogue_code.back().assembly_text += " ; Restore caller's Frame Pointer";
    epilogue_code.push_back(Encoder::create_ldr_imm("X30", "SP", 8));
    epilogue_code.back().assembly_text += " ; Restore Link Register";
    epilogue_code.push_back(Encoder::create_add_imm("SP", "SP", this->final_frame_size));
    epilogue_code.back().assembly_text += " ; Deallocate space for saved FP/LR";
    
    // 6. The single, standard return instruction.
    epilogue_code.push_back(Encoder::create_return());

    // Only add failure handlers if stack canaries are enabled
    if (enable_stack_canaries) {
        // Place the failure handlers *after* the normal return path.
        // Upper canary failure handler:
        Instruction upper_label_instr;
        upper_label_instr.is_label_definition = true;
        upper_label_instr.target_label = function_name + "_stackprot_upper";
        epilogue_code.push_back(upper_label_instr); // Only semantic info, no assembly text.
        epilogue_code.push_back(Encoder::create_brk(0)); // Halt execution
        epilogue_code.back().assembly_text += " ; Stack Corruption Detected for Upper Canary!";

        // Lower canary failure handler:
        Instruction lower_label_instr;
        lower_label_instr.is_label_definition = true;
        lower_label_instr.target_label = function_name + "_stackprot_lower";
        epilogue_code.push_back(lower_label_instr); // Only semantic info, no assembly text.
        epilogue_code.push_back(Encoder::create_brk(0)); // Halt execution
        epilogue_code.back().assembly_text += " ; Stack Corruption Detected for Lower Canary!";
    }

    return epilogue_code;
}
