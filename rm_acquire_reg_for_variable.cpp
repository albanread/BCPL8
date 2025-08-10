#include "RegisterManager.h"
#include "NewCodeGenerator.h"
#include "Encoder.h"
#include <stdexcept>

std::pair<std::string, bool> RegisterManager::acquire_reg_for_variable(const std::string& variable_name, NewCodeGenerator& code_gen) {
    // CACHE HIT: Variable is already in a register
    if (variable_to_reg_map.count(variable_name)) {
        std::string reg = variable_to_reg_map.at(variable_name);
        // Update LRU status: move to front
        variable_reg_lru_order_.remove(reg);
        variable_reg_lru_order_.push_front(reg);
        return {reg, true}; // Return register and 'true' for hit
    }

    // CACHE MISS: Find a free register or spill one
    std::string reg = find_free_register(VARIABLE_REGS);
    if (!reg.empty()) {
        registers[reg] = {IN_USE_VARIABLE, variable_name, false};
        variable_to_reg_map[variable_name] = reg;
        variable_reg_lru_order_.push_front(reg); // Add to LRU tracking
        return {reg, false}; // Return new register and 'false' for miss
    }

    // Spill logic: No free registers, so we must spill the least recently used one.
    std::string victim_reg = variable_reg_lru_order_.back();
    variable_reg_lru_order_.pop_back();

    std::string spilled_var = registers.at(victim_reg).bound_to;

    // --- THE CHANGE IS HERE: ONLY SPILL IF DIRTY ---
    if (registers.at(victim_reg).dirty) {
        Instruction spill_instr = generate_spill_code(victim_reg, spilled_var, *code_gen.get_current_frame_manager());
        code_gen.emit(spill_instr);
        // Mark as clean after spilling
        registers.at(victim_reg).dirty = false;
        // Mark old variable as spilled and remove it from the register map
        spilled_variables_.insert(spilled_var);
        variable_to_reg_map.erase(spilled_var);
    } else {
        // If not dirty, just evict the variable from the register
        variable_to_reg_map.erase(spilled_var);
        spilled_variables_.insert(spilled_var);
    }
    // --- END OF CHANGE ---

    // Assign the now-free victim register to the new variable
    registers[victim_reg] = {IN_USE_VARIABLE, variable_name, false};
    variable_to_reg_map[variable_name] = victim_reg;
    variable_reg_lru_order_.push_front(victim_reg);
    spilled_variables_.erase(variable_name); // Ensure the newly acquired variable is not marked as spilled

    return {victim_reg, false};
}