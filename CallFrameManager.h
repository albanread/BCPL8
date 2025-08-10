#ifndef CALL_FRAME_MANAGER_H
// FLET implementation for floating-point declarations
#define CALL_FRAME_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <memory> // For std::unique_ptr
#include <map> // For std::map
#include "Encoder.h"
#include "DataTypes.h"

// Forward declaration of RegisterManager to avoid circular dependency issues
class RegisterManager;

class CallFrameManager {
public:
    // Constructor: Takes a reference to the RegisterManager
    CallFrameManager(RegisterManager& register_manager, const std::string& function_name = "", bool debug = false);

    // Informs the manager of a new local variable that needs space on the stack.
    void add_local(const std::string& variable_name, int size_in_bytes = 8);
    void add_parameter(const std::string& name);

    // Retrieves the stack offset for a previously declared local variable.
    int get_offset(const std::string& variable_name) const;

    // Set the type of a variable (for dynamic temporaries)
    void set_variable_type(const std::string& variable_name, VarType type);

    // Allocates a spill slot for a variable and returns its offset
    int get_spill_offset(const std::string& variable_name);

    // Accessor for the function name
    const std::string& get_function_name() const { return function_name; }

    // Returns a vector of all local variable names
    std::vector<std::string> get_local_variable_names() const {
        std::vector<std::string> names;
        for (const auto& it : variable_offsets) {
            names.push_back(it.first);
        }
        return names;
    }

    // Explicitly forces a register to be saved/restored, regardless of locals.
    void force_save_register(const std::string& reg_name);

    // Predictively reserve callee-saved registers based on register pressure.
    void reserve_registers_based_on_pressure(int register_pressure);

    // Constructs the complete assembly instruction sequence for the function prologue.
    std::vector<Instruction> generate_prologue();

    // Constructs the complete assembly instruction sequence for the function epilogue.
    std::vector<Instruction> generate_epilogue();

    // Checks if a local variable with the given name exists.
    bool has_local(const std::string& variable_name) const;

    // Mark a variable as floating-point type
    void mark_variable_as_float(const std::string& variable_name);
    
    // Check if a variable is a floating-point type
    bool is_float_variable(const std::string& variable_name) const;

    // Generates a human-readable string representing the current state of the stack frame.
    std::string display_frame_layout() const;

    // New getter for the dedicated X29 spill slot offset.
    int get_x29_spill_slot_offset() const;

private:
    void debug_print(const std::string& message) const;
    int align_to_16(int size) const;

    RegisterManager& reg_manager; // Reference to the RegisterManager singleton
    bool debug_enabled;
    std::string function_name; // Stores the name of the function

    // **NEW:** Tracks the current accumulated size of locals for provisional offset assignment.
    int current_locals_offset_ = 16; // Start locals after FP/LR spill slots (+0, +8)

    // Local variable tracking
    struct LocalVar {
        std::string name;
        int size;

        // Constructor to initialize LocalVar with name and size
        LocalVar(const std::string& variable_name, int variable_size)
            : name(variable_name), size(variable_size) {}

    private:
        // Spill-related member variables moved to CallFrameManager class
        int spill_area_size_; // Total size of the spill area
        int next_spill_offset_; // Offset for the next spill slot
        std::map<std::string, int> spill_variable_offsets_; // Maps variable names to spill offsets
    };
    std::vector<LocalVar> local_declarations;
    int locals_total_size;

    // Final layout information
    std::unordered_map<std::string, int> variable_offsets; // Maps variable name to its final offset from FP
    int spill_area_size_; // Total size of the spill area
    int next_spill_offset_; // Offset for the next spill slot
    std::map<std::string, int> spill_variable_offsets_; // Maps variable names to spill offsets
    std::unordered_map<std::string, bool> float_variables_; // Tracks which variables are floating-point types

public:
    // Pre-allocate spill slots before prologue generation
    void preallocate_spill_slots(int count);

private:
    int final_frame_size;
    std::vector<std::string> callee_saved_registers_to_save;
    bool is_prologue_generated;

    // The final calculated offset for our dedicated slot.
    int x29_spill_slot_offset;

    // Stack Canary values
    static constexpr uint64_t UPPER_CANARY_VALUE = 0x1122334455667788ULL;
    static constexpr uint64_t LOWER_CANARY_VALUE = 0xAABBCCDDEEFF0011ULL;
    // Size of each canary in bytes
    static constexpr int CANARY_SIZE = 8; // Canaries are 64-bit (8 bytes)
    // Flag to enable/disable stack canaries (disabled by default)
    static bool enable_stack_canaries;

public:
    // Static method to enable or disable stack canaries
    static void setStackCanariesEnabled(bool enabled);
    
private:
    // Get canary size based on whether they're enabled or not
    int getCanarySize() const;
};

#endif // CALL_FRAME_MANAGER_H
