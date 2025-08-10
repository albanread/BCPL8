#ifndef CODE_LISTER_H
#define CODE_LISTER_H

#include "Encoder.h"
#include "DataGenerator.h"
#include "DataTypes.h"
#include <vector>
#include <string>

/**
 * @brief A utility to generate a human-readable listing of code and data.
 *
 * This class takes the finalized output from the Linker and DataGenerator
 * and creates a formatted string that shows addresses, hex encodings,
 * and the assembly text, which is invaluable for debugging.
 */
class CodeLister {
public:
    CodeLister() = default;

    /**
     * @brief Generates a complete assembly listing for a function's code.
     * @param instructions The finalized vector of instructions from the Linker.
     * @param defined_labels A map of label names to their addresses for annotation.
     * @return A formatted, multi-line string representing the code listing.
     */
    std::string generate_code_listing(
        const std::vector<Instruction>& instructions,
        const std::unordered_map<std::string, size_t>& defined_labels,
        size_t base_address = 0
    ) const;

    /**
     * @brief Generates a listing for the data sections.
     * @param data_generator The DataGenerator containing all static data.
     * @return A formatted, multi-line string representing the data listing.
     */
    std::string generate_data_listing(const DataGenerator& data_generator) const;

    /**
     * @brief Generates a hex/ASCII dump of the data sections.
     * @param data_generator The DataGenerator containing all static data.
     * @param data_section_base_address The base address where the data section starts in memory.
     * @return A formatted, multi-line string representing the data dump.
     */
    std::string generate_data_dump(const DataGenerator& data_generator, size_t data_section_base_address) const;
};

#endif // CODE_LISTER_H
