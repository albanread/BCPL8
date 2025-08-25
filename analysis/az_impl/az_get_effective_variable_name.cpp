#include "../ASTAnalyzer.h"
#include <stack>
#include <map>
#include <string>

/**
 * @brief Resolves the effective variable name, handling FOR loop variable renaming.
 * @param original_name The original variable name.
 * @return The effective (possibly renamed) variable name.
 */
std::string ASTAnalyzer::get_effective_variable_name(const std::string& original_name) const {
    // 1. First, check the active loop scopes for nested loops.
    // This ensures that an inner `FOR I` correctly shadows an outer `FOR I`.
    std::stack<std::map<std::string, std::string>> temp_stack = active_for_loop_scopes_;
    while (!temp_stack.empty()) {
        const auto& scope_map = temp_stack.top();
        auto it = scope_map.find(original_name);
        if (it != scope_map.end()) {
            return it->second; // Found in an active scope
        }
        temp_stack.pop();
    }

    // 2. If not in an active loop, check the persistent aliases map.
    // This allows access to the variable after the loop has finished.
    auto alias_it = for_variable_unique_aliases_.find(original_name);
    if (alias_it != for_variable_unique_aliases_.end()) {
        return alias_it->second; // Return the last known unique name for this variable
    }

    // 3. If it was never a loop variable, return its original name.
    return original_name;
}