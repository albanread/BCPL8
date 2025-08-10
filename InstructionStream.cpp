#include "InstructionStream.h"
#include <stdexcept>
#include <iostream>
#include <sstream>

InstructionStream::InstructionStream(LabelManager& label_manager)
    : label_manager_(label_manager) {}

// CORRECTED: This method is now "dumb" and does not set any address.
void InstructionStream::add(const Instruction& instr) {
    instructions_.push_back(instr);
    // Set the address on the new copy that is now inside the vector
    instructions_.back().address = 0;
}

// CORRECTED: This method now adds a pseudo-instruction for the Linker to process.
void InstructionStream::define_label(const std::string& label_name) {
    Instruction label_instr;
    label_instr.is_label_definition = true; // Mark this as a label definition
    label_instr.target_label = label_name;  // Store the label name
    instructions_.push_back(label_instr);   // Add it to the stream
}

// This helper is no longer needed as define_label handles it.
// You can remove create_and_define_label if you wish.
std::string InstructionStream::create_and_define_label() {
    std::string label_name = label_manager_.create_label();
    define_label(label_name);
    return label_name;
}


// --- NO CHANGES NEEDED FOR THE METHODS BELOW ---

void InstructionStream::add_data32(uint32_t data, const std::string& label_name, SegmentType segment) {
    std::stringstream ss;
    ss << "DCD 0x" << std::hex << data;
    Instruction instr(data, ss.str());
    instr.is_data_value = true;
    instr.segment = segment;
    if (!label_name.empty()) {
        instr.target_label = label_name;
    }
    add(instr);
}

void InstructionStream::add_data64(uint64_t data, const std::string& label_name, SegmentType segment) {
    uint32_t lower = static_cast<uint32_t>(data & 0xFFFFFFFF);
    uint32_t upper = static_cast<uint32_t>(data >> 32);

    std::stringstream ss;
    ss << ".quad 0x" << std::hex << data;

    Instruction quad_instr;
    quad_instr.encoding = lower;
    quad_instr.assembly_text = ss.str();
    quad_instr.is_data_value = true;
    quad_instr.segment = segment;
    if (!label_name.empty()) {
        quad_instr.target_label = label_name;
        quad_instr.is_label_definition = true; // Ensure label is defined for linker
    }
    add(quad_instr);

    std::string upper_comment = "; (upper half)";
    Instruction upper_instr;
    upper_instr.encoding = upper;
    upper_instr.assembly_text = upper_comment;
    upper_instr.is_data_value = true;
    upper_instr.segment = segment;
    add(upper_instr);
}

void InstructionStream::add_padcode(int alignment) {
    if (alignment != 8 && alignment != 16) {
        throw std::runtime_error("add_padcode alignment must be 8 or 16 bytes.");
    }
    const uint32_t NOP_ENCODING = 0xD503201F;
    Instruction nop_instruction(NOP_ENCODING, "nop");
    size_t current_size_bytes = instructions_.size() * 4;
    size_t remainder = current_size_bytes % alignment;
    if (remainder == 0) return;
    size_t padding_bytes = alignment - remainder;
    size_t nop_count = padding_bytes / 4;
    for (size_t i = 0; i < nop_count; ++i) {
        add(nop_instruction);
    }
}

// Pads data sections with 32-bit zeros to a given byte alignment.
void InstructionStream::add_data_padding(int alignment) {
    if (alignment <= 0) return;
    size_t current_size_bytes = instructions_.size() * 4;
    size_t remainder = current_size_bytes % alignment;
    if (remainder == 0) return;
    size_t padding_bytes = alignment - remainder;
    size_t zero_count = padding_bytes / 4;
    for (size_t i = 0; i < zero_count; ++i) {
        add_data32(0);
    }
}

/**
 * @brief Returns a copy of the internal instruction vector.
 */
std::vector<Instruction> InstructionStream::get_instructions() const {
    return instructions_;
}

/**
 * @brief Replaces the internal instruction vector with a new one.
 */
void InstructionStream::replace_instructions(const std::vector<Instruction>& new_instructions) {
    instructions_ = new_instructions;
}

/**
 * @brief Returns an estimation of the current address in bytes.
 * This is used for branch offset calculations when checking if a function
 * is within range for direct branching.
 * 
 * Each regular instruction takes 4 bytes.
 * 64-bit data values take 8 bytes.
 * Labels don't take any actual bytes.
 */
size_t InstructionStream::get_current_address() const {
    size_t address = 0;
    for (const auto& instr : instructions_) {
        if (instr.is_data_value) {
            // We currently have two types of data values:
            // 32-bit (4 bytes) and 64-bit (8 bytes, represented as two 32-bit values)
            // The add_data64 method adds two instructions for a single 64-bit value
            address += 4; // All data entries currently take 4 bytes each
        } else if (!instr.is_label_definition) {
            // Regular instruction
            address += 4;
        }
        // Labels don't contribute to the address
    }
    return address;
}
