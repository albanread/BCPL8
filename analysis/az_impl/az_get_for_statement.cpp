#include "../ASTAnalyzer.h"
#include <stdexcept>

/**
 * @brief Retrieves a ForStatement node using its unique loop variable name.
 * @param unique_name The unique name generated for the loop variable.
 * @return Reference to the corresponding ForStatement.
 * @throws std::runtime_error if the unique name is not found.
 */
const ForStatement& ASTAnalyzer::get_for_statement(const std::string& unique_name) const {
    auto it = for_statements_.find(unique_name);
    if (it != for_statements_.end()) {
        return *(it->second);
    }
    throw std::runtime_error("Analyzer failed to find ForStatement for unique name: " + unique_name);
}