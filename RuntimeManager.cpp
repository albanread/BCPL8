#include "RuntimeManager.h"
#include "LabelManager.h"
#include <cstdio>
#include <algorithm>
#include <iostream>
#include <cstdint>

RuntimeManager::RuntimeManager() {}

std::string RuntimeManager::to_upper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

void RuntimeManager::register_function(const std::string& name, int num_args, void* address, FunctionType type) {
    std::string upper_name = to_upper(name);
    if (functions_.count(upper_name)) {
        throw std::runtime_error("Runtime function '" + name + "' already registered.");
    }
    // --- START OF MODIFICATION ---
    size_t offset = next_table_offset_;
    next_table_offset_ += 8; // Each pointer is 8 bytes (64-bit)
    if ((offset / 8) >= 256) {
        throw std::runtime_error("Exceeded pre-allocated runtime function table size (256 entries).");
    }
    RuntimeFunction func(upper_name, num_args, address, type);
    func.table_offset = offset;
    functions_.emplace(upper_name, func);
    // --- END OF MODIFICATION ---
}

const RuntimeFunction& RuntimeManager::get_function(const std::string& name) const {
    std::string upper_name = to_upper(name);
    auto it = functions_.find(upper_name);
    if (it == functions_.end()) {
        throw std::runtime_error("Runtime function '" + name + "' not found.");
    }
    return it->second;
}

bool RuntimeManager::is_function_registered(const std::string& name) const {
    std::string upper_name = to_upper(name);
    return functions_.count(upper_name) > 0;
}



void RuntimeManager::set_function_address(const std::string& name, void* address) {
    std::string upper_name = to_upper(name);
    auto it = functions_.find(upper_name);
    if (it == functions_.end()) {
        throw std::runtime_error("Runtime function '" + name + "' not found.");
    }
    it->second.address = address;
}

void RuntimeManager::print_registered_functions() const {
    printf("=== Registered Runtime Functions ===\n");
    for (const auto& kv : functions_) {
        const auto& func = kv.second;
        printf("  %-16s | address: %p | args: %d | table_offset: %zu\n", func.name.c_str(), func.address, func.num_args, func.table_offset);
    }
    printf("====================================\n");
}

// Populate the X28-relative runtime function pointer table in the .data segment
void RuntimeManager::populate_function_pointer_table(void* data_segment_base) const {
    if (!data_segment_base) {
        throw std::runtime_error("Cannot populate runtime table with a null data segment base.");
    }

    // Get the map of all registered runtime functions
    const auto& functions = get_registered_functions();

    for (const auto& pair : functions) {
        const RuntimeFunction& func = pair.second;

        // Calculate the destination address inside the .data segment table
        uint64_t* destination_ptr = reinterpret_cast<uint64_t*>(
            static_cast<char*>(data_segment_base) + func.table_offset
        );

        // Write the 64-bit absolute address of the function into the table
        *destination_ptr = reinterpret_cast<uint64_t>(func.address);
    }

    if (isTracingEnabled()) {
        std::cout << "JIT runtime table populated with " << functions.size() << " function pointers." << std::endl;
    }
}

// ADD THIS ENTIRE NEW METHOD
size_t RuntimeManager::get_function_offset(const std::string& name) const {
    std::string upper_name = to_upper(name);
    auto it = functions_.find(upper_name);
    if (it == functions_.end()) {
        throw std::runtime_error("Runtime function '" + name + "' not found when getting offset.");
    }
    return it->second.table_offset;
}
