#include "CallFrameManager.h"
#include "RegisterManager.h"
#include "Encoder.h"
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <vector>

std::vector<Instruction> CallFrameManager::generate_prologue() {
    if (is_prologue_generated) {
        throw std::runtime_error("Prologue already generated.");
    }

    debug_print("Starting prologue generation.");

    // --- ADD THIS NEW BLOCK ---
    if (uses_global_pointers_) {
        debug_print("Function uses global pointers. Forcing save of X19 and X28.");
        // Unconditionally add X19 and X28 to the save list if they aren't already there.
        if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), "X19") == callee_saved_registers_to_save.end()) {
            callee_saved_registers_to_save.push_back("X19");
        }
        if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), "X28") == callee_saved_registers_to_save.end()) {
            callee_saved_registers_to_save.push_back("X28");
        }
    }
    // --- END OF NEW BLOCK ---

    // Sort registers to ensure a consistent stack layout.
    std::sort(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end());
    debug_print("Sorted callee_saved_registers_to_save.");

    // --- Calculate Frame Size ---
    int callee_saved_regs_count = this->callee_saved_registers_to_save.size();
    int callee_saved_size = callee_saved_regs_count * 8;
    int minimum_frame_content = 16;

    int canary_space = enable_stack_canaries ? (2 * CANARY_SIZE) : 0;
    int required_content_size = this->locals_total_size + callee_saved_size + minimum_frame_content + canary_space + spill_area_size_;

    this->final_frame_size = this->align_to_16(required_content_size); // Ensure alignment includes spill area

    // --- Assign Offsets ---
    int current_offset_for_vars = 16 + (enable_stack_canaries ? (2 * CANARY_SIZE) : 0);
    this->variable_offsets.clear();

    for (const auto& decl : this->local_declarations) {
        variable_offsets[decl.name] = current_offset_for_vars;
        current_offset_for_vars += decl.size;
    }

    for (const auto& reg_name : this->callee_saved_registers_to_save) {
        if (variable_offsets.find(reg_name) == variable_offsets.end()) {
             variable_offsets[reg_name] = current_offset_for_vars;
             current_offset_for_vars += 8;
        }
    }

    // --- FIX: Initialize the starting offset for the spill area ---
    // This ensures spills use the first free address after all locals and saved registers.
    this->next_spill_offset_ = current_offset_for_vars;

    // Only needed when stack canaries are enabled
    int upper_canary_offset = 16;
    int lower_canary_offset = 16 + CANARY_SIZE;

    // --- Generate Prologue Code ---
    std::vector<Instruction> prologue_code;

    prologue_code.push_back(Encoder::create_stp_pre_imm("X29", "X30", "SP", -this->final_frame_size));
    prologue_code.push_back(Encoder::create_mov_fp_sp());

    // Only add stack canaries if enabled
    if (enable_stack_canaries) {
        for (const auto& instr : Encoder::create_movz_movk_abs64("X9", UPPER_CANARY_VALUE, "")) {
            prologue_code.push_back(instr);
        }
        prologue_code.back().assembly_text += " ; Load UPPER_CANARY_VALUE";
        prologue_code.push_back(Encoder::create_str_imm("X9", "X29", upper_canary_offset));
        prologue_code.back().assembly_text += " ; Store Upper Stack Canary";

        for (const auto& instr : Encoder::create_movz_movk_abs64("X9", LOWER_CANARY_VALUE, "")) {
            prologue_code.push_back(instr);
        }
        prologue_code.back().assembly_text += " ; Load LOWER_CANARY_VALUE";
        prologue_code.push_back(Encoder::create_str_imm("X9", "X29", lower_canary_offset));
        prologue_code.back().assembly_text += " ; Store Lower Stack Canary";
    }

    for (const auto& reg : this->callee_saved_registers_to_save) {
        int offset = variable_offsets.at(reg);
        Instruction instr = Encoder::create_str_imm(reg, "X29", offset);
        instr.assembly_text += " ; Saved Reg: " + reg + " @ FP+" + std::to_string(offset);
        prologue_code.push_back(instr);
    }

    this->is_prologue_generated = true;

    if (debug_enabled) {
        std::cout << display_frame_layout() << std::endl;
    }

    return prologue_code;
}
