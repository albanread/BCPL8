#include "DataGenerator.h"
#include "InstructionStream.h"
#include <sstream>
#include <stdexcept>
#include <codecvt>
#include <locale>
#include <cstring>
#include <iostream> // For std::cerr
#include "DataTypes.h"
#include "LabelManager.h" // Required for the new listing functions
#include "RuntimeManager.h" // Required for tracing functionality
#include <iomanip>    // Required for std::setw, std::left
#include <vector>     // Required for std::vector
#include <algorithm>  // Required for std::sort

// Helper function to convert std::string (UTF-8) to std::u32string (UTF-32)
static std::u32string utf8_to_utf32(const std::string& utf8_str) {
    try {
        std::u32string result;
        for (size_t i = 0; i < utf8_str.size();) {
            char32_t codepoint = 0;
            unsigned char byte = utf8_str[i];
            if (byte <= 0x7F) {
                codepoint = byte;
                i += 1;
            } else if ((byte & 0xE0) == 0xC0) {
                codepoint = ((utf8_str[i] & 0x1F) << 6) | (utf8_str[i + 1] & 0x3F);
                i += 2;
            } else if ((byte & 0xF0) == 0xE0) {
                codepoint = ((utf8_str[i] & 0x0F) << 12) | ((utf8_str[i + 1] & 0x3F) << 6) | (utf8_str[i + 2] & 0x3F);
                i += 3;
            } else if ((byte & 0xF8) == 0xF0) {
                codepoint = ((utf8_str[i] & 0x07) << 18) | ((utf8_str[i + 1] & 0x3F) << 12) | ((utf8_str[i + 2] & 0x3F) << 6) | (utf8_str[i + 3] & 0x3F);
                i += 4;
            } else {
                throw std::runtime_error("Invalid UTF-8 sequence");
            }
            result.push_back(codepoint);
        }
        return result;
    } catch (const std::range_error& e) {
        // Handle conversion errors, e.g., invalid UTF-8 sequence
        throw std::runtime_error("UTF-8 to UTF-32 conversion error: " + std::string(e.what()));
    }
}

DataGenerator::DataGenerator(bool enable_tracing)
    : next_string_id_(0), next_float_id_(0), enable_tracing_(enable_tracing) {}

std::string DataGenerator::add_string_literal(const std::string& value) {
    if (string_literal_map_.count(value)) {
        return string_literal_map_[value];
    }

    std::string label = "L_str" + std::to_string(next_string_id_++);
    string_literal_map_[value] = label;

    // Convert input UTF-8 string to UTF-32
    std::u32string u32_value = utf8_to_utf32(value);

    // As per the spec, add two 32-bit zero terminators.
    u32_value.push_back(U'\0');
    u32_value.push_back(U'\0');

    if (enable_tracing_) {
        fprintf(stderr, "[DEBUG] DataGenerator::add_string_literal: UTF-32 value size = %zu\n", u32_value.length());
        for (size_t i = 0; i < u32_value.length(); ++i) {
            fprintf(stderr, "[DEBUG] DataGenerator::add_string_literal: u32_value[%zu] = 0x%X\n", i, u32_value[i]);
        }
    }

    string_literals_.push_back({label, u32_value});

    if (enable_tracing_) {
        std::cerr << "DEBUG: DataGenerator: Added string literal: " << label << " with value: " << value << std::endl;
    }

    return label;
}

std::string DataGenerator::add_float_literal(double value) {
    if (float_literal_map_.count(value)) {
        return float_literal_map_[value];
    }

    std::string label = "L_float" + std::to_string(next_float_id_++);
    float_literal_map_[value] = label;

    float_literals_.push_back({label, value});

    return label;
}

void DataGenerator::add_global_variable(const std::string& name, ExprPtr initializer) {
    static_variables_.push_back({name, std::move(initializer)});
}

void DataGenerator::calculate_global_offsets() {
    global_word_offsets_.clear();
    size_t current_word_offset = 0;
    for (const auto& info : static_variables_) {
        // Assign the next available 64-bit word slot.
        global_word_offsets_[info.label] = current_word_offset++;
    }
}

size_t DataGenerator::get_global_word_offset(const std::string& name) const {
    auto it = global_word_offsets_.find(name);
    if (it == global_word_offsets_.end()) {
        throw std::runtime_error("Global variable '" + name + "' has no calculated offset.");
    }
    return it->second;
}



bool DataGenerator::is_global_variable(const std::string& name) const {
    // This check is sufficient. If it was added as a static, it's a global.
    return std::any_of(static_variables_.begin(), static_variables_.end(),
                       [&](const auto& var) { return var.label == name; });
}

/**
 * @brief Generates the read-only data section (.rodata) directly into the stream.
 * @param stream The instruction stream to add the data to.
 */
void DataGenerator::generate_rodata_section(InstructionStream& stream) {
    if (string_literals_.empty() && float_literals_.empty()) {
        return;
    }

    // Process all string literals according to the specification.
    for (const auto& info : string_literals_) {
        // 1. Pad to 16-byte alignment before each string entry.
        stream.add_data_padding(16);

        // Emit a label definition instruction for the string literal
        Instruction label_instr;
        label_instr.is_label_definition = true;
        label_instr.target_label = info.label;
        label_instr.segment = SegmentType::RODATA;
        stream.add(label_instr);

        // 2. Write the string length (DCD strlen). Length is the number of
        //    characters *before* the two null terminators were added.
        size_t string_len = info.value.length() - 2;

        // --- Emit the length as the first 32-bit word directly into the stream.
        stream.add_data32(static_cast<uint32_t>(string_len), "", SegmentType::RODATA);

        // --- Emit each 32-bit character code as a separate data word.
        for (char32_t c : info.value) {
            stream.add_data32(static_cast<uint32_t>(c), "", SegmentType::RODATA);
        }
    }

    // Add final 4x DCD 0 termination for the rodata segment if strings were present.
    if (!string_literals_.empty()) {
        // Emit final 4x DCD 0 as a single horizontal line
        std::stringstream ss;
        ss << "DCD 0x0, 0x0, 0x0, 0x0";
        Instruction pad_instr;
        pad_instr.assembly_text = ss.str();
        pad_instr.segment = SegmentType::RODATA;
        stream.add(pad_instr);
    }

    // Process all float literals.
    for (const auto& info : float_literals_) {
        // Align each double to an 8-byte boundary.
        stream.add_padcode(8);

        // 1. Create and add the label definition instruction.
        Instruction label_instr;
        label_instr.is_label_definition = true;
        label_instr.target_label = info.label;
        label_instr.segment = SegmentType::RODATA;
        stream.add(label_instr);

        // 2. Add the 64-bit data for the float.
        uint64_t float_bits;
        std::memcpy(&float_bits, &info.value, sizeof(uint64_t));

        // Now that the label is defined separately, the data itself doesn't need the label name.
        stream.add_data64(float_bits, "", SegmentType::RODATA);
    }
}

/**
 * @brief Generates the initialized data section (.data) directly into the stream.
 * @param stream The instruction stream to add the data to.
 */
void DataGenerator::generate_data_section(InstructionStream& stream) {
    calculate_global_offsets();

    if (static_variables_.empty()) {
        return;
    }

    bool base_label_emitted = false;
    for (const auto& info : static_variables_) {
        if (info.initializer) {
            std::cerr << "[DEBUG GLOBAL] Initializer type for " << info.label << ": " << typeid(info.initializer.get()).name() << std::endl;
        } else {
            std::cerr << "[DEBUG GLOBAL] No initializer for " << info.label << std::endl;
        }
        NumberLiteral* num_lit = dynamic_cast<NumberLiteral*>(info.initializer.get());
        long initial_value = num_lit ? num_lit->int_value : 0;
        std::cerr << "[DEBUG GLOBAL] Emitting value for " << info.label << ": " << initial_value << std::endl;

        // Emit L__data_segment_base label directly before the first labelled global variable
        if (!base_label_emitted) {
            Instruction label_instr;
            label_instr.is_label_definition = true;
            label_instr.target_label = "L__data_segment_base";
            label_instr.segment = SegmentType::DATA;
            stream.add(label_instr);
            base_label_emitted = true;
        }

        // The linker will place this at the next available 8-byte boundary,
        // which is guaranteed since the base is aligned and all items are 8 bytes.
        stream.add_data64(static_cast<uint64_t>(initial_value), info.label, SegmentType::DATA);
    }
}

// --- Segment display methods ---

/**
 * @brief Generates a formatted string listing the contents of the .rodata section.
 * @param label_manager The label manager containing the final addresses of data labels.
 * @return A string containing the formatted .rodata listing.
 */
std::string DataGenerator::generate_rodata_listing(const LabelManager& label_manager) const {
    if (!RuntimeManager::instance().isTracingEnabled()) {
        return ""; // Suppress output if tracing is disabled
    }

    std::stringstream ss;
    ss << "\n--- JIT .rodata Segment Dump ---\n";
    ss << std::left << std::setw(20) << "Label"
       << " @ Address"
       << " | len"
       << " | String\n";

    if (string_literals_.empty() && float_literals_.empty()) {
        ss << "(No read-only data generated)\n";
    }

    // --- String Literals ---
    for (const auto& info : string_literals_) {
        if (label_manager.is_label_defined(info.label)) {
            size_t address = label_manager.get_label_address(info.label);
            size_t len = info.value.length() - 2;

            // Decode the string from UTF-32 to UTF-8 for display, showing control chars as <LF>, <CR>, <TAB>, etc.
            std::string decoded;
            for (size_t i = 0; i < len; ++i) {
                char32_t cp = info.value[i];
                if (cp == 0x0A) {
                    decoded += "<LF>";
                } else if (cp == 0x0D) {
                    decoded += "<CR>";
                } else if (cp == 0x09) {
                    decoded += "<TAB>";
                } else if (cp == 0x0B) {
                    decoded += "<VT>";
                } else if (cp == 0x0C) {
                    decoded += "<FF>";
                } else if (cp == 0x00) {
                    decoded += "<NUL>";
                } else if (cp < 0x20) {
                    decoded += "<CTRL>";
                } else if (cp <= 0x7F) {
                    decoded += static_cast<char>(cp);
                } else if (cp <= 0x7FF) {
                    decoded += static_cast<char>(0xC0 | ((cp >> 6) & 0x1F));
                    decoded += static_cast<char>(0x80 | (cp & 0x3F));
                } else if (cp <= 0xFFFF) {
                    decoded += static_cast<char>(0xE0 | ((cp >> 12) & 0x0F));
                    decoded += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    decoded += static_cast<char>(0x80 | (cp & 0x3F));
                } else {
                    decoded += static_cast<char>(0xF0 | ((cp >> 18) & 0x07));
                    decoded += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
                    decoded += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
                    decoded += static_cast<char>(0x80 | (cp & 0x3F));
                }
            }

            ss << std::left << std::setw(20) << info.label
               << " @ 0x" << std::hex << address
               << " | len: " << std::dec << len
               << " | " << decoded << std::endl;
        }
    }

    // --- Float Literals ---
    for (const auto& info : float_literals_) {
        if (label_manager.is_label_defined(info.label)) {
            size_t address = label_manager.get_label_address(info.label);
            ss << std::left << std::setw(20) << info.label
               << " @ 0x" << std::hex << address
               << " | value: " << std::fixed << std::setprecision(6) << info.value << std::dec << std::endl;
        }
    }
    ss << "------------------------------\n";
    return ss.str();
}

/**
 * @brief Dumps the contents of the read-write .data segment for debugging.
 * @param label_manager The label manager containing the final addresses of data labels.
 * @param data_base_address The runtime base address of the .data segment.
 * @return A string containing the formatted .data listing.
 */

/**
 * @brief Populates the JIT data segment with initial values for global variables.
 * @param data_base_address The base address of the JIT data segment.
 * @param label_manager The label manager containing the final addresses of data labels.
 */
void DataGenerator::populate_data_segment(void* data_base_address, const LabelManager& label_manager) const {
    for (const auto& info : static_variables_) {
        if (!label_manager.is_label_defined(info.label)) {
            std::cerr << "[DEBUG GLOBAL] Label not defined for global: " << info.label << std::endl;
            continue;
        }
        uintptr_t address = label_manager.get_label_address(info.label);
        // The address is an absolute address in the JIT data memory.
        // Write the initial value (handle only integer NumberLiteral for now).
        uint64_t initial_value = 0;
        if (info.initializer) {
            if (NumberLiteral* num_lit = dynamic_cast<NumberLiteral*>(info.initializer.get())) {
                initial_value = static_cast<uint64_t>(num_lit->int_value);
            } else {
                std::cerr << "[DEBUG GLOBAL] Initializer for " << info.label << " is not a NumberLiteral. Initializing to 0." << std::endl;
            }
        }
        std::memcpy(reinterpret_cast<void*>(address), &initial_value, sizeof(uint64_t));
        std::cerr << "[DEBUG GLOBAL] Wrote value " << initial_value << " to address 0x" << std::hex << address << std::dec << " for global " << info.label << std::endl;
    }
}
std::string DataGenerator::generate_data_listing(const LabelManager& label_manager, void* data_base_address) const {
    if (!RuntimeManager::instance().isTracingEnabled()) {
        return ""; // Suppress output if tracing is disabled
    }

    std::stringstream ss;
    ss << "\n--- JIT .data Segment Dump ---\n";
    
    if (static_variables_.empty()) {
        ss << "(No global read-write variables defined)\n";
    }

    for (const auto& info : static_variables_) {
        if (label_manager.is_label_defined(info.label)) {
            uintptr_t address = label_manager.get_label_address(info.label);
            // Read the final value directly from the JIT's data memory
            uint64_t value = *reinterpret_cast<uint64_t*>(address);

            ss << std::left << std::setw(20) << info.label
               << " @ 0x" << std::hex << address
               << " | Value: " << std::dec << static_cast<int64_t>(value)
               << " (0x" << std::hex << value << std::dec << ")" << std::endl;
        }
    }
    ss << "----------------------------\n";
    return ss.str();
}
