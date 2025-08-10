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
    std::stack<std::map<std::string, std::string>> temp_stack = active_for_loop_scopes_;
    while (!temp_stack.empty()) {
        const auto& scope_map = temp_stack.top();
        auto it = scope_map.find(original_name);
        if (it != scope_map.end()) {
            return it->second;
        }
        temp_stack.pop();
    }
    return original_name;
}