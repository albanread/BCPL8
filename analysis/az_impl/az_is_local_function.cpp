#include "../ASTAnalyzer.h"

/**
 * @brief Checks if a given name corresponds to a known user-defined function.
 * @param name The name to check.
 * @return True if the name is in the set of discovered local function names.
 */
bool ASTAnalyzer::is_local_function(const std::string& name) const {
    return local_function_names_.count(name) > 0;
}