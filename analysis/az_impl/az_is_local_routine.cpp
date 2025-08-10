
#include "../ASTAnalyzer.h"

/**
 * @brief Checks if a given name corresponds to a known user-defined routine.
 * @param name The name to check.
 * @return True if the name is in the set of discovered local routine names.
 */
bool ASTAnalyzer::is_local_routine(const std::string& name) const {
    return local_routine_names_.count(name) > 0;
}
