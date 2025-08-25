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
#include <set>

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

void CallFrameManager::set_active_register_pool(bool use_extended_pool) {
    if (use_extended_pool) {
        active_variable_regs_ = &RegisterManager::EXTENDED_VARIABLE_REGS;
    } else {
        active_variable_regs_ = &RegisterManager::VARIABLE_REGS;
    }
}

CallFrameManager::CallFrameManager(RegisterManager& register_manager, const std::string& function_name, bool debug)
    : reg_manager(register_manager),
      locals_total_size(0),
      final_frame_size(0),
      is_prologue_generated(false),
      x29_spill_slot_offset(0),
      debug_enabled(debug),
      function_name(function_name),
      current_locals_offset_(16 + (enable_stack_canaries ? (2 * CANARY_SIZE) : 0)),
      spill_area_size_(0),
      next_spill_offset_(0)
{
    if (debug_enabled) {
        std::cout << "Call Frame Layout for function: " << function_name << "\n";
    }
}

// Helper to get the size for a given VarType
size_t CallFrameManager::get_size_for_type(VarType type) const {
    switch (type) {
        case VarType::POINTER_TO_FLOAT_VEC:
        case VarType::POINTER_TO_INT_VEC:
            return 16;
        case VarType::FLOAT:
        case VarType::INTEGER:
        case VarType::POINTER_TO_ANY_LIST:
        case VarType::POINTER_TO_INT_LIST:
        case VarType::POINTER_TO_FLOAT_LIST:
        case VarType::POINTER_TO_LIST_NODE:
        case VarType::POINTER_TO_STRING:
        case VarType::POINTER_TO_TABLE:
        case VarType::POINTER_TO_FLOAT:
        case VarType::POINTER_TO_INT:
            return 8;
        default:
            return 8;
    }
}

// Add a local variable with type
void CallFrameManager::add_local(const std::string& variable_name, VarType type) {
    if (variable_offsets.count(variable_name)) {
        // Already added
        return;
    }
    size_t size = get_size_for_type(type);
    variable_offsets[variable_name] = current_locals_offset_;
    variable_types_[variable_name] = type;
    current_locals_offset_ += size;
    locals_total_size += size;
    local_declarations.push_back(LocalVar(variable_name, size));
    if (debug_enabled) {
        std::cout << "Added local variable '" << variable_name << "' of type " << static_cast<int>(type)
                  << " at offset " << variable_offsets[variable_name] << " (size " << size << ")\n";
    }
}

// Add a parameter with type
void CallFrameManager::add_parameter(const std::string& name, VarType type) {
    // For now, treat parameters like locals
    add_local(name, type);
}

int CallFrameManager::get_spill_offset(const std::string& variable_name) {
    if (spill_variable_offsets_.count(variable_name)) {
        return spill_variable_offsets_.at(variable_name);
    }
    // Use the type to determine slot size
    VarType type = VarType::UNKNOWN;
    auto it = variable_types_.find(variable_name);
    if (it != variable_types_.end()) {
        type = it->second;
    }
    int slot_size = static_cast<int>(get_size_for_type(type));
    int offset = next_spill_offset_;
    spill_variable_offsets_[variable_name] = offset;
    next_spill_offset_ += slot_size;
    spill_area_size_ += slot_size;
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

    debug_print("Analyzing register reservation based on pressure: " + std::to_string(register_pressure));

    // Iterate through the ACTIVE register pool, not a hardcoded range.
    for (int i = 0; i < register_pressure; ++i) {
        if (i >= active_variable_regs_->size()) {
            debug_print("Warning: Register pressure exceeds the available callee-saved pool size.");
            break;
        }

        const std::string& reg_name = (*active_variable_regs_)[i];

        // Mark the register for saving, ensuring it's added only once.
        if (std::find(callee_saved_registers_to_save.begin(), callee_saved_registers_to_save.end(), reg_name) == callee_saved_registers_to_save.end()) {
            callee_saved_registers_to_save.push_back(reg_name);
            debug_print("Reserved (and marked for saving) " + reg_name + " due to register pressure.");
        }
    }
}

// Set the type of a variable (for dynamic temporaries)
void CallFrameManager::set_variable_type(const std::string& variable_name, VarType type) {
    variable_types_[variable_name] = type;
}

VarType CallFrameManager::get_variable_type(const std::string& variable_name) const {
    auto it = variable_types_.find(variable_name);
    return it != variable_types_.end() ? it->second : VarType::UNKNOWN;
}



// ... (other methods as needed, except has_local which is now only in cf_has_local.cpp)

