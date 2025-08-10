#include "Encoder.h"
#include <sstream>

Instruction Encoder::create_directive(const std::string& directive_text, uint64_t data_value, const std::string& target_label, bool is_data) {
    // For directives, the encoding is the full 64-bit data_value.
    // The assembly_text holds the directive itself, ensuring clarity in the output.
    std::stringstream ss;
    ss << ".quad 0x" << std::hex << data_value;

    // Append the target label as a comment for clarity, if provided.
    if (!target_label.empty()) {
        ss << " ; " << target_label;
    }

    // Return the instruction with the full directive text and data value.
    Instruction instr(static_cast<uint32_t>(data_value & 0xFFFFFFFF), ss.str(), RelocationType::NONE, target_label, is_data, false);
    instr.opcode = InstructionDecoder::OpType::DIRECTIVE;
    return instr;
}
