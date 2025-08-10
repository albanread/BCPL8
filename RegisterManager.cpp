#include "RegisterManager.h"

const std::vector<std::string> RegisterManager::VARIABLE_REGS = {"X21", "X22", "X23", "X24", "X25", "X26", "X27"};
const std::vector<std::string> RegisterManager::FP_VARIABLE_REGS = {
    "D8", "D9", "D10", "D11", "D12", "D13", "D14", "D15"
};
const std::vector<std::string> RegisterManager::FP_SCRATCH_REGS = {
    "D0", "D1", "D2", "D3", "D4", "D5", "D6", "D7",
    "D16", "D17", "D18", "D19", "D20"
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

    for (const auto& reg : VARIABLE_REGS) registers[reg] = {FREE, "", false};
    for (const auto& reg : SCRATCH_REGS) registers[reg] = {FREE, "", false};
    for (const auto& reg : RESERVED_REGS) registers[reg] = {IN_USE_DATA_BASE, "data_base", false};
    for (const auto& reg : FP_VARIABLE_REGS) registers[reg] = {FREE, "", false};
    for (const auto& reg : FP_SCRATCH_REGS) registers[reg] = {FREE, "", false};
    fp_variable_to_reg_map_.clear();
    fp_variable_reg_lru_order_.clear();

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











// --- Codegen Helper Methods ---

std::string RegisterManager::get_free_register() {
    return acquire_scratch_reg();
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