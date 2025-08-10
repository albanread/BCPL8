#include "RuntimeManager.h"
#include "LabelManager.h"
#include <cstdio>
#include <algorithm>

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
    functions_.emplace(upper_name, RuntimeFunction(upper_name, num_args, address, type));
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
        printf("  %-16s | address: %p | args: %d\n", func.name.c_str(), func.address, func.num_args);
    }
    printf("====================================\n");}
