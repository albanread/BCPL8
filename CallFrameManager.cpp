#include "CallFrameManager.h"
#include "RegisterManager.h"
#include "Encoder.h"
#include "DataTypes.h"
#include <iostream>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <unordered_set>
#include <stdexcept>
#include <numeric>
#include <string>
#include <vector>

// Initialize static class members
bool CallFrameManager::enable_stack_canaries = false;

// Static method to enable or disable stack canaries
void CallFrameManager::setStackCanariesEnabled(bool enabled) {
    enable_stack_canaries = enabled;
}

// Get canary size based on whether they're enabled or not
int CallFrameManager::getCanarySize() const {
    return enable_stack_canaries ? CANARY_SIZE : 0;
}

CallFrameManager::CallFrameManager(RegisterManager& register_manager, const std::string& function_name, bool debug)
    : reg_manager(register_manager),
      locals_total_size(0),
      final_frame_size(0),
      is_prologue_generated(false),
      x29_spill_slot_offset(0), // Will effectively be 0 with current AArch64 STP/LDP.
      debug_enabled(debug),
      function_name(function_name), // Initialize function name
      current_locals_offset_(16 + (enable_stack_canaries ? (2 * CANARY_SIZE) : 0)), // Start locals after FP/LR and canaries if enabled
    spill_area_size_(0),
    next_spill_offset_(0) {
    if (debug_enabled) {
        std::cout << "Call Frame Layout for function: " << function_name << "\n";
    }
}


    int CallFrameManager::get_spill_offset(const std::string& variable_name) {
        if (spill_variable_offsets_.count(variable_name)) {
            return spill_variable_offsets_.at(variable_name);
        }
        int offset = next_spill_offset_;
        spill_variable_offsets_[variable_name] = offset;
        next_spill_offset_ += 8;
        spill_area_size_ += 8;
        return offset;
    }

    // Pre-allocate spill slots before prologue generation
    void CallFrameManager::preallocate_spill_slots(int count) {
        if (is_prologue_generated) {
            // Should not happen with correct logic
            return;
        }
        int bytes_to_add = count * 8; // 8 bytes per spill slot
        spill_area_size_ += bytes_to_add;
        debug_print("Pre-allocated " + std::to_string(count) + " spill slots (" + std::to_string(bytes_to_add) + " bytes).");
    }

void CallFrameManager::force_save_register(const std::string& reg_name) {
    if (is_prologue_generated) {
        throw std::runtime_error("Cannot force saving register after prologue is generated.");
    }
    // Only add it if it's not already in the list
    if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), reg_name) == callee_saved_registers_to_save.end()) {
        callee_saved_registers_to_save.push_back(reg_name);
        if (debug_enabled) {
            std::cout << "Added " << reg_name << " to callee_saved_registers_to_save list." << std::endl;
        }
    }
}

// Predicts and reserves callee-saved registers based on register pressure.
// Assumes the register allocation strategy uses X21 upwards for variables/temporaries.
// Marks these registers for saving in the prologue.
void CallFrameManager::reserve_registers_based_on_pressure(int register_pressure) {
    // Ensure this is called before the prologue is finalized.
    if (is_prologue_generated) {
        throw std::runtime_error("Cannot reserve registers after prologue is generated.");
    }

    // Define the pool of registers used by the allocator for general variables/temporaries.
    // Based on trace analysis, the strategy uses X21 upwards.
    const int START_REG = 21;
    // X28 is the maximum general-purpose callee-saved register (X29=FP, X30=LR).
    const int END_REG = 28;

    debug_print("Analyzing register reservation based on pressure: " + std::to_string(register_pressure));

    for (int i = 0; i < register_pressure; ++i) {
        int reg_num = START_REG + i;

        if (reg_num > END_REG) {
            // If pressure exceeds the available callee-saved pool (X21-X28),
            // the allocator will need to adapt (use caller-saved or spill).
            // We cap the reservation here.
            debug_print("Warning: Register pressure exceeds the assumed callee-saved pool (X21-X28).");
            break;
        }

        std::string reg_name = "X" + std::to_string(reg_num);

        // Mark the register for saving, ensuring it's added only once.
        if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), reg_name) == callee_saved_registers_to_save.end()) {
            callee_saved_registers_to_save.push_back(reg_name);
            debug_print("Reserved (and marked for saving) " + reg_name + " due to register pressure.");
        }
    }
}

// Set the type of a variable (for dynamic temporaries)
void CallFrameManager::set_variable_type(const std::string& variable_name, VarType type) {
    debug_print("Setting type for '" + variable_name + "'");
    if (type == VarType::FLOAT) {
        float_variables_[variable_name] = true;
    } else {
        // If it was previously marked as float, unmark it
        if (float_variables_.count(variable_name)) {
            float_variables_.erase(variable_name);
        }
    }
}
