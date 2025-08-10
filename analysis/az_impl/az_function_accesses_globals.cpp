#include "../ASTAnalyzer.h"

/**
 * @brief Returns true if the given function accesses global variables.
 */
bool ASTAnalyzer::function_accesses_globals(const std::string& function_name) const {
    if (function_metrics_.count(function_name)) {
        return function_metrics_.at(function_name).accesses_globals;
    }
    return false;
}