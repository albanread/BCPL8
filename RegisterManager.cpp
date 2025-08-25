#include "RegisterManager.h"

const std::vector<std::string> RegisterManager::VARIABLE_REGS = {"X20", "X21", "X22", "X23", "X24", "X25", "X26", "X27"};
const std::vector<std::string> RegisterManager::EXTENDED_VARIABLE_REGS = {
    "X19", "X20", "X21", "X22", "X23", "X24", "X25", "X26", "X27", "X28"
};
const std::vector<std::string> RegisterManager::FP_VARIABLE_REGS = {
    "D8", "D9", "D10", "D11", "D12", "D13", "D14", "D15"
};
const std::vector<std::string> RegisterManager::FP_SCRATCH_REGS = {
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "D16", "D17", "D18", "D19", "D20",
    "D21", "D22", "D23", "D24", "D25", "D26", "D27", "D28", "D29", "D30", "D31"
};

// --- Vector Register Pools ---
const std::vector<std::string> RegisterManager::VEC_VARIABLE_REGS = {
    "V8", "V9", "V10", "V11", "V12", "V13", "V14", "V15"
};
const std::vector<std::string> RegisterManager::VEC_SCRATCH_REGS = {
    "V0", "V1", "V2", "V3", "V4", "V5", "V6", "V7",
    "V16", "V17", "V18", "V19", "V20", "V21", "V22", "V23",
    "V24", "V25", "V26", "V27", "V28", "V29", "V30", "V31"
};
#include "NewCodeGenerator.h" // Needed for spill/reload codegen access
#include "CallFrameManager.h" // Needed for spill/reload codegen access
#include "Encoder.h"          // Needed for spill/reload codegen access
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <list>
#include <unordered_set>
#include <unordered_map>

// Singleton instance
RegisterManager* RegisterManager::instance_ = nullptr;

RegisterManager::RegisterManager() {
    initialize_registers();
}

RegisterManager& RegisterManager::getInstance() {
    if (!instance_) {
        instance_ = new RegisterManager();
    }
    return *instance_;
}

void RegisterManager::reset() {
    initialize_registers();
}

void RegisterManager::reset_for_new_function(bool accesses_globals) {
    initialize_registers();
    if (accesses_globals) {
        // Function needs X19/X28 for their special roles, so use the smaller pool.
        active_variable_regs_ = &VARIABLE_REGS;
    } else {
        // Function is pure computation; make X19 and X28 available for allocation.
        active_variable_regs_ = &EXTENDED_VARIABLE_REGS;
    }
}

bool RegisterManager::is_variable_spilled(const std::string& variable_name) const {
    return spilled_variables_.count(variable_name) > 0;
}

void RegisterManager::initialize_registers() {
    registers.clear();
    variable_to_reg_map.clear();
    variable_reg_lru_order_.clear();
    spilled_variables_.clear();
    
    // Initialize all registers as clean (not dirty)
    for (const auto& reg : VARIABLE_REGS) {
        registers[reg].dirty = false;
    }
    for (const auto& reg : SCRATCH_REGS) {
        registers[reg].dirty = false;
    }

    for (const auto& reg : FP_VARIABLE_REGS) {
        registers[reg].dirty = false;
    }
    for (const auto& reg : FP_SCRATCH_REGS) {
        registers[reg].dirty = false;
    }

    // Initialize all managed registers to FREE

    // --- THE FIX ---
    // Initialize the state for the SUPERSET of all possible variable registers.
    // This ensures that whether the active pool is small or extended, the
    // register state is always valid and present in the map.
    for (const auto& reg : EXTENDED_VARIABLE_REGS) registers[reg] = {FREE, "", false};
    // --- END FIX ---
    for (const auto& reg : SCRATCH_REGS) registers[reg] = {FREE, "", false};
    for (const auto& reg : RESERVED_REGS) registers[reg] = {IN_USE_DATA_BASE, "data_base", false};
    for (const auto& reg : FP_VARIABLE_REGS) registers[reg] = {FREE, "", false};
    for (const auto& reg : FP_SCRATCH_REGS) registers[reg] = {FREE, "", false};
    // Vector register initialization
    for (const auto& reg : VEC_VARIABLE_REGS) registers[reg] = {FREE, "", false};
    for (const auto& reg : VEC_SCRATCH_REGS) registers[reg] = {FREE, "", false};
    fp_variable_to_reg_map_.clear();
    fp_variable_reg_lru_order_.clear();
    vec_variable_to_reg_map_.clear();
    vec_variable_reg_lru_order_.clear();

}

// --- New Cache Management Logic ---

// --- Floating-point register allocation ---









std::string RegisterManager::get_free_float_register() {
    return acquire_fp_scratch_reg();
}





bool RegisterManager::is_scratch_register(const std::string& register_name) const {
    // This is a simplified check. A more robust implementation would use the SCRATCH_REGS vector.
    for (const auto& reg : SCRATCH_REGS) {
        if (reg == register_name) return true;
    }
    return false;
}







// --- Helper Functions ---

std::string RegisterManager::find_free_register(const std::vector<std::string>& pool) {
    for (const auto& reg : pool) {
        if (registers.at(reg).status == FREE) {
            return reg;
        }
    }
    return "";
}

Instruction RegisterManager::generate_spill_code(const std::string& reg_name, const std::string& variable_name, CallFrameManager& cfm) {
    // If the register is not dirty (hasn't been modified since loading), we can skip the store
    if (!is_dirty(reg_name)) {
        // Return an empty instruction (no-op) to indicate no spill needed
        return Instruction(0, "// Skipping spill for clean register " + reg_name + " (" + variable_name + ")");
    }
    
    // Otherwise generate the store instruction for the dirty register
    int offset = cfm.get_spill_offset(variable_name);
    return Encoder::create_str_imm(reg_name, "X29", offset, variable_name);
}


// --- Variable & Scratch Management (Using Partitioned Pools) ---










// --- ABI & State Management ---

// Updated acquire_scratch_reg with spill-on-demand logic
std::string RegisterManager::acquire_scratch_reg(NewCodeGenerator& code_gen) {
    // 1. First, try to find a free register in the primary scratch pool.
    std::string reg = find_free_register(SCRATCH_REGS);
    if (!reg.empty()) {
        registers[reg] = {IN_USE_SCRATCH, "scratch", false};
        return reg;
    }

    // 2. If scratch pool is full, try to find a free register in the callee-saved variable pool.
    reg = find_free_register(VARIABLE_REGS);
    if (!reg.empty()) {
        registers[reg] = {IN_USE_SCRATCH, "scratch_from_vars", false};
        return reg;
    }

    // 3. If both pools are full, spill the least recently used variable register.
    if (variable_reg_lru_order_.empty()) {
        throw std::runtime_error("No scratch registers available and no variable registers to spill.");
    }

    std::string victim_reg = variable_reg_lru_order_.back();
    variable_reg_lru_order_.pop_back();

    std::string spilled_var = registers.at(victim_reg).bound_to;

    // Only generate a store instruction if the register is dirty.
    if (registers.at(victim_reg).dirty) {
        Instruction spill_instr = generate_spill_code(victim_reg, spilled_var, *code_gen.get_current_frame_manager());
        code_gen.emit(spill_instr);
    }

    // Update state: mark the old variable as spilled and remove its register mapping.
    variable_to_reg_map.erase(spilled_var);
    spilled_variables_.insert(spilled_var);

    // Assign the now-free victim register for scratch use.
    registers[victim_reg] = {IN_USE_SCRATCH, "scratch_from_spill", false};
    
    return victim_reg;
}

// --- Vector Register Management ---

// Acquire a vector scratch register (caller-saved)
std::string RegisterManager::acquire_vec_scratch_reg() {
    std::string reg = find_free_register(VEC_SCRATCH_REGS);
    if (!reg.empty()) {
        registers[reg] = {IN_USE_SCRATCH, "vec_scratch", false};
        return reg;
    }
    throw std::runtime_error("No available vector scratch registers.");
}

void RegisterManager::release_vec_scratch_reg(const std::string& reg_name) {
    if (registers.count(reg_name) && registers.at(reg_name).status == IN_USE_SCRATCH) {
        registers[reg_name] = {FREE, "", false};
    }
}

// Acquire a vector variable register (callee-saved, with LRU spill)
std::string RegisterManager::acquire_vec_variable_reg(const std::string& variable_name, NewCodeGenerator& code_gen, CallFrameManager& cfm) {
    // 1. Cache Hit: Variable is already in a register
    if (vec_variable_to_reg_map_.count(variable_name)) {
        std::string reg = vec_variable_to_reg_map_.at(variable_name);
        vec_variable_reg_lru_order_.remove(variable_name);
        vec_variable_reg_lru_order_.push_front(variable_name);
        return reg;
    }

    // 2. Cache Miss: Find a free register
    std::string reg = find_free_register(VEC_VARIABLE_REGS);
    if (!reg.empty()) {
        registers[reg] = {IN_USE_VARIABLE, variable_name, false};
        vec_variable_to_reg_map_[variable_name] = reg;
        vec_variable_reg_lru_order_.push_front(variable_name);
        return reg;
    }

    // 3. Spill: No free registers, spill the least recently used one.
    std::string victim_var = vec_variable_reg_lru_order_.back();
    vec_variable_reg_lru_order_.pop_back();
    std::string victim_reg = vec_variable_to_reg_map_[victim_var];
    
    // Generate spill code (uses 128-bit STR)
    int offset = cfm.get_offset(victim_var); // Assumes CFM handles 16-byte slots
    // NOTE: You will need to implement Encoder::create_str_vec_imm for actual codegen.
    // code_gen.emit(Encoder::create_str_vec_imm(victim_reg, "X29", offset));

    // Update state
    vec_variable_to_reg_map_.erase(victim_var);
    registers[victim_reg] = {IN_USE_VARIABLE, variable_name, false};
    vec_variable_to_reg_map_[variable_name] = victim_reg;
    vec_variable_reg_lru_order_.push_front(variable_name);

    return victim_reg;
}











// --- Codegen Helper Methods ---

std::string RegisterManager::get_free_register(NewCodeGenerator& code_gen) {
    return acquire_scratch_reg(code_gen);
}











std::string RegisterManager::get_zero_register() const {
    return "XZR";
}

void RegisterManager::mark_register_as_used(const std::string& reg_name) {
    // This could be used to track usage for more advanced register allocation.
}



// Implementation for invalidating caller-saved registers


bool RegisterManager::is_fp_register(const std::string& reg_name) const {
    // Check if the register is in the FP variable or scratch pools
    for (const auto& reg : FP_VARIABLE_REGS) {
        if (reg == reg_name) return true;
    }
    for (const auto& reg : FP_SCRATCH_REGS) {
        if (reg == reg_name) return true;
    }
    return false;
}