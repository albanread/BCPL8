#ifndef ASSEMBLY_WRITER_H
#define ASSEMBLY_WRITER_H

#include "Encoder.h" // For Instruction struct
#include "LabelManager.h" // For LabelManager (to get defined labels)
#include "DataGenerator.h" // For DataGenerator
#include <string>
#include <vector>
#include <fstream>
#include "CodeLister.h"

// The AssemblyWriter writes a human-readable assembly file compatible with standard toolchains.
class AssemblyWriter {
public:
    AssemblyWriter();

    // Writes the finalized instructions to an assembly file.
    // The label_manager is needed to know where labels are defined.
    void write_to_file(
        const std::string& path,
        const std::vector<Instruction>& instructions,
        const LabelManager& label_manager,
        const DataGenerator& data_generator
    );

private:
    // Helper to write assembler directives
    void write_directives(std::ofstream& ofs);
};

#endif // ASSEMBLY_WRITER_H
