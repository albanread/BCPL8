#include "CodeLister.h"
#include <sstream>
#include <iomanip>
#include <algorithm>

std::string CodeLister::generate_code_listing(
    const std::vector<Instruction>& instructions,
    const std::unordered_map<std::string, size_t>& defined_labels,
    size_t base_address) const
{
    if (instructions.empty()) {
        return "";
    }

    std::stringstream ss;
    ss << "--- Code Section Listing (Base Address: 0x" << std::hex << base_address << std::dec << ") ---\n";
    ss << std::left << std::setw(8) << "Address"
       << std::setw(12) << "Hex Code"
       << "Assembly\n";
    ss << "--------------------------------------------------\n";

    // Create a reverse map from address to label name for easy lookup
    std::unordered_map<size_t, std::string> address_to_label;
    for (const auto& pair : defined_labels) {
        // The linker now provides absolute addresses for labels
        address_to_label[pair.second] = pair.first;
    }

    for (size_t i = 0; i < instructions.size(); ++i) {
        const auto& instr = instructions[i];

        // *** THE FIX IS HERE ***
        // The linker has already calculated the absolute address. We just use it directly.
        size_t absolute_address = instr.address;

        // Check if a label is defined at this instruction's absolute address
        if (address_to_label.count(absolute_address)) {
            ss << address_to_label[absolute_address] << ":\n";
        }

        // Format the line: Address | Hex | Assembly
        ss << " "
           << std::setw(7) << std::hex << absolute_address << std::dec << " ";

        if (instr.encoding == 0x0 && instr.assembly_text.empty()) {
            ss << "0x00000000  DCD 0\n";
            continue;
        }
        ss << "0x" << std::hex << std::setw(8) << std::setfill('0') << instr.encoding << std::dec << "  ";
        ss << instr.assembly_text;

        // After printing the assembly_text, check if the linker patched this instruction.
        if (instr.relocation_applied) {
            ss << "    ; Reloc -> '" << instr.resolved_symbol_name << "' @ 0x" << std::hex << instr.resolved_target_address << std::dec;
        }

        ss << "\n";
    }

    ss << "\n";
    return ss.str();
}
