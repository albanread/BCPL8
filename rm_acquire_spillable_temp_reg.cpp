#include "RegisterManager.h"
#include "NewCodeGenerator.h"
#include "Encoder.h"
#include <stdexcept>

std::string RegisterManager::acquire_spillable_temp_reg(NewCodeGenerator& code_gen) {
    // First, try to find a free register in the variable pool.
    std::string reg = find_free_register(VARIABLE_REGS);
    if (!reg.empty()) {
        registers[reg] = {IN_USE_VARIABLE, "_temp_", false}; // Bind to a temporary name
        variable_reg_lru_order_.push_front(reg);
        return reg;
    }

    // If no registers are free, spill the least recently used one.
    std::string victim_reg = variable_reg_lru_order_.back();
    variable_reg_lru_order_.pop_back();
    std::string spilled_var = registers.at(victim_reg).bound_to;

    if (registers.at(victim_reg).dirty) {
        Instruction spill_instr = generate_spill_code(victim_reg, spilled_var, *code_gen.get_current_frame_manager());
        code_gen.emit(spill_instr);
    }

    variable_to_reg_map.erase(spilled_var);
    spilled_variables_.insert(spilled_var);

    // Assign the now-free register for our temporary use.
    registers[victim_reg] = {IN_USE_VARIABLE, "_temp_", false};
    variable_reg_lru_order_.push_front(victim_reg);

    return victim_reg;
}