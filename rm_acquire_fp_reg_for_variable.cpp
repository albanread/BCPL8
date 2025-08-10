#include "RegisterManager.h"
#include "NewCodeGenerator.h"
#include "CallFrameManager.h"
#include "Encoder.h"
#include <stdexcept>

std::string RegisterManager::acquire_fp_reg_for_variable(const std::string& variable_name, NewCodeGenerator& code_gen, CallFrameManager& cfm) {
    // If already allocated, return it
    if (fp_variable_to_reg_map_.count(variable_name)) {
        return fp_variable_to_reg_map_[variable_name];
    }
    // Find a free FP variable register
    for (const auto& reg : FP_VARIABLE_REGS) {
        if (registers[reg].status == FREE) {
            registers[reg].status = IN_USE_VARIABLE;
            registers[reg].bound_to = variable_name;
            fp_variable_to_reg_map_[variable_name] = reg;
            fp_variable_reg_lru_order_.push_front(variable_name);
            return reg;
        }
    }
    // If none free, spill the least recently used
    if (!fp_variable_reg_lru_order_.empty()) {
        std::string victim_var = fp_variable_reg_lru_order_.back();
        fp_variable_reg_lru_order_.pop_back();
        std::string victim_reg = fp_variable_to_reg_map_[victim_var];
        int offset = cfm.get_spill_offset(victim_var);
        Instruction spill_instr = Encoder::create_str_fp_imm(victim_reg, "X29", offset);
        code_gen.emit(spill_instr);
        registers[victim_reg].status = FREE;
        registers[victim_reg].bound_to = "";

        fp_variable_to_reg_map_.erase(victim_var);
        // Allocate for new variable
        registers[victim_reg].status = IN_USE_VARIABLE;
        registers[victim_reg].bound_to = variable_name;
        fp_variable_to_reg_map_[variable_name] = victim_reg;
        fp_variable_reg_lru_order_.push_front(variable_name);
        return victim_reg;
    }
    throw std::runtime_error("No available FP variable registers and cannot spill.");
}