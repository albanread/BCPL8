#ifndef REGISTER_MANAGER_H
#define REGISTER_MANAGER_H

#include <string>
#include <vector>
#include <unordered_map>
#include <list>
#include <unordered_set>
#include "Encoder.h"

// Forward declarations
class NewCodeGenerator;
class CallFrameManager;

class RegisterManager {
public:
    // --- FP Register Pools ---
    static const std::vector<std::string> FP_VARIABLE_REGS;
    static const std::vector<std::string> FP_SCRATCH_REGS;

    // New pools for 128-bit Vector Registers
    static const std::vector<std::string> VEC_VARIABLE_REGS;  // Callee-saved (V8-V15)
    static const std::vector<std::string> VEC_SCRATCH_REGS;   // Caller-saved (V0-V7, V16-V31)

    // --- Singleton Access Method ---
    static RegisterManager& getInstance();

    // Check if a register is a scratch register
    bool is_scratch_register(const std::string& register_name) const;




    // --- Variable & Scratch Management (Updated to use partitioned pools) ---
    std::pair<std::string, bool> acquire_reg_for_variable(const std::string& variable_name, NewCodeGenerator& code_gen);
    void release_reg_for_variable(const std::string& variable_name);
    std::string acquire_scratch_reg(NewCodeGenerator& code_gen);
    void release_scratch_reg(const std::string& reg_name);

    // Acquire/release a callee-saved temp register for preserving values across function calls
    std::string acquire_callee_saved_temp_reg(CallFrameManager& cfm);
    void release_callee_saved_temp_reg(const std::string& reg_name);

    // NEW METHOD:
    // Acquires a register for a temporary, anonymous value,
    // using the spillable VARIABLE_REGS pool.
    std::string acquire_spillable_temp_reg(NewCodeGenerator& code_gen);
    std::string acquire_spillable_fp_temp_reg(NewCodeGenerator& code_gen);

    // Resets only caller-saved/scratch registers (used in routine call codegen)
    void reset_caller_saved_registers();

    // --- Helper methods for code generation compatibility ---
    std::string get_free_register(NewCodeGenerator& code_gen);
    void release_register(const std::string& reg_name);
    std::string get_free_float_register();

    // --- Floating-point register allocation ---
    std::string acquire_fp_reg_for_variable(const std::string& variable_name, NewCodeGenerator& code_gen, CallFrameManager& cfm);
    std::string acquire_fp_scratch_reg();
    void release_fp_register(const std::string& reg_name);
    std::vector<std::string> get_in_use_fp_callee_saved_registers() const;
    std::vector<std::string> get_in_use_fp_caller_saved_registers() const;

    // --- Vector Register Management ---
    std::string acquire_vec_scratch_reg();
    void release_vec_scratch_reg(const std::string& reg_name);
    std::string acquire_vec_variable_reg(const std::string& variable_name, NewCodeGenerator& code_gen, CallFrameManager& cfm);

    // --- Floating-point register management ---
    std::unordered_map<std::string, std::string> fp_variable_to_reg_map_;
    std::list<std::string> fp_variable_reg_lru_order_;

    // --- Vector Register Tracking ---
    std::unordered_map<std::string, std::string> vec_variable_to_reg_map_;
    std::list<std::string> vec_variable_reg_lru_order_;


    // --- Additional stubs for codegen compatibility ---
    std::string get_zero_register() const;

    void mark_register_as_used(const std::string& reg_name);

    // --- ABI & State Management ---
    void mark_dirty(const std::string& reg_name, bool is_dirty = true);
    bool is_dirty(const std::string& reg_name) const;
    std::vector<std::pair<std::string, std::string>> get_dirty_variable_registers() const;
    std::vector<std::string> get_in_use_callee_saved_registers() const;
    std::vector<std::string> get_in_use_caller_saved_registers() const;
    // --- ADD THIS NEW METHOD ---
    void reset_for_new_function(bool accesses_globals);

    void reset();
    void invalidate_caller_saved_registers();

    bool is_fp_register(const std::string& reg_name) const;
    bool is_variable_spilled(const std::string& variable_name) const;

private:
    // Private constructor to prevent direct instantiation
    RegisterManager();
    // Private destructor (optional, but good practice for singletons if specific cleanup is needed)
    ~RegisterManager() = default;

    // Delete copy constructor and assignment operator to prevent copying
    RegisterManager(const RegisterManager&) = delete;
    RegisterManager& operator=(const RegisterManager&) = delete;

    static RegisterManager* instance_;



public:
    enum RegisterStatus {
        FREE,
        IN_USE_VARIABLE,
        IN_USE_SCRATCH,
        IN_USE_ROUTINE_ADDR,
        IN_USE_DATA_BASE
    };

    struct RegisterInfo {
        RegisterStatus status;
        std::string bound_to;
        bool dirty;
    };


    std::unordered_map<std::string, RegisterInfo> registers;
    std::unordered_map<std::string, std::string> variable_to_reg_map;
    std::list<std::string> variable_reg_lru_order_;

    // Stores the list of caller-saved GP and FP registers spilled for restoration
    std::vector<std::string> caller_saved_spills_;
    std::vector<std::string> fp_caller_saved_spills_;

    // For saving/restoring caller-saved state



    const std::string DATA_BASE_REG = "X28";
    static const std::vector<std::string> VARIABLE_REGS;
    static const std::vector<std::string> EXTENDED_VARIABLE_REGS;

    // Pointer to the currently active pool
    const std::vector<std::string>* active_variable_regs_ = &VARIABLE_REGS;
    const std::vector<std::string> SCRATCH_REGS = {"X10", "X11", "X12", "X13", "X14", "X15"};
    const std::vector<std::string> RESERVED_REGS = {DATA_BASE_REG};

    std::unordered_set<std::string> spilled_variables_;

    void initialize_registers();
    std::string find_free_register(const std::vector<std::string>& pool);
    Instruction generate_spill_code(const std::string& reg_name, const std::string& variable_name, CallFrameManager& cfm);
};

#endif // REGISTER_MANAGER_H
