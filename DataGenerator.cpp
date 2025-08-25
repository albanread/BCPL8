#include "DataGenerator.h"
#include "InstructionStream.h"
#include <sstream>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include "DataTypes.h"
#include "LabelManager.h"
#include <vector>
#include <algorithm>
#include <iomanip>
#include "AST.h"
#include "runtime/ListDataTypes.h" // For ATOM_SENTINEL

// --- Helper: Emit 64-bit absolute relocatable pointer as two instructions ---
static void emit_absolute_pointer(InstructionStream& stream, const std::string& label, SegmentType segment) {
    Instruction lo32, hi32;
    lo32.relocation = RelocationType::ABSOLUTE_ADDRESS_LO32;
    hi32.relocation = RelocationType::ABSOLUTE_ADDRESS_HI32;
    lo32.target_label = label;
    hi32.target_label = label;
    lo32.segment = segment;
    hi32.segment = segment;
    lo32.is_data_value = true;
    hi32.is_data_value = true;
    lo32.assembly_text = ".quad " + label + " ; (lo32)";
    hi32.assembly_text = "; (hi32)";
    stream.add(lo32);
    stream.add(hi32);
}

// --- Helper: Canonical key for list literals ---
static std::string generate_list_key(const ListExpression* node) {
    std::ostringstream oss;
    oss << "list[";
    bool first = true;
    for (const auto& expr : node->initializers) {
        if (!first) oss << ",";
        first = false;
        if (auto* num_lit = dynamic_cast<NumberLiteral*>(expr.get())) {
            if (num_lit->literal_type == NumberLiteral::LiteralType::Integer) {
                oss << "int(" << num_lit->int_value << ")";
            } else {
                oss << "float(" << num_lit->float_value << ")";
            }
        } else if (auto* str_lit = dynamic_cast<StringLiteral*>(expr.get())) {
            oss << "string(" << str_lit->value << ")";
        } else if (auto* list_lit = dynamic_cast<ListExpression*>(expr.get())) {
            oss << generate_list_key(list_lit);
        } else {
            oss << "unknown";
        }
    }
    oss << "]";
    return oss.str();
}

// Note: utf8_to_utf32 helper function remains the same as provided previously.
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
        throw std::runtime_error("UTF-8 to UTF-32 conversion error: " + std::string(e.what()));
    }
}


DataGenerator::DataGenerator(bool enable_tracing)
    : next_string_id_(0),
      next_float_id_(0),
      next_list_id_(0),
      next_table_id_(0),
      next_float_table_id_(0),
      enable_tracing_(enable_tracing) {}

// add_string_literal, add_float_literal, etc. remain the same as provided previously...
std::string DataGenerator::add_string_literal(const std::string& value) {
    if (string_literal_map_.count(value)) {
        return string_literal_map_[value];
    }
    std::string label = "L_str" + std::to_string(next_string_id_++);
    string_literal_map_[value] = label;
    std::u32string u32_value = utf8_to_utf32(value);
    u32_value.push_back(U'\0');
    u32_value.push_back(U'\0');
    string_literals_.push_back({label, u32_value});
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

// Other add methods like add_table_literal...
std::string DataGenerator::add_table_literal(const std::vector<ExprPtr>& initializers) {
    std::string label = "L_tbl" + std::to_string(next_table_id_++);
    std::vector<int64_t> values;
    for (const auto& expr : initializers) {
        if (auto* num_lit = dynamic_cast<NumberLiteral*>(expr.get())) {
            if (num_lit->literal_type == NumberLiteral::LiteralType::Integer) {
                values.push_back(num_lit->int_value);
            } else {
                throw std::runtime_error("TABLE initializers must be integer literals.");
            }
        } else {
            throw std::runtime_error("TABLE initializers must be constant integer literals.");
        }
    }
    table_literals_.push_back({label, values});
    return label;
}


std::string DataGenerator::add_list_literal(const ListExpression* node) {
    // 1. Generate and immediately print the key to see what it is.
    std::string canonical_key = generate_list_key(node);
    if (enable_tracing_) {
        std::cout << "[DataGenerator TRACE] add_list_literal called. Generated Key: \"" << canonical_key << "\"" << std::endl;
    }

    // Memoization: Use canonical key based on content, not pointer.
    auto it = list_literal_label_map.find(canonical_key);
    if (it != list_literal_label_map.end()) {
        // 2. Print a message indicating a cache hit.
        if (enable_tracing_) {
            std::cout << "[DataGenerator TRACE] >> Cache HIT. Reusing label: " << it->second << std::endl;
        }
        return it->second;
    }

    // 3. Print a message indicating a cache miss and that new labels will be created.
    if (enable_tracing_) {
        std::cout << "[DataGenerator TRACE] >> Cache MISS. Generating new labels." << std::endl;
    }

    // This is the "Collect Phase". It gathers all information needed for emission.
    ListLiteralInfo list_info;
    list_info.length = node->initializers.size();

    std::string base_label = "L_list" + std::to_string(next_list_id_++);
    list_info.header_label = base_label + "_header";

    std::vector<std::string> node_labels;
    for (size_t i = 0; i < node->initializers.size(); ++i) {
        node_labels.push_back(base_label + "_node_" + std::to_string(i));
    }

    for (size_t i = 0; i < node->initializers.size(); ++i) {
        auto& expr = node->initializers[i];
        ListLiteralNode node_data;
        node_data.node_label = node_labels[i];
        node_data.next_node_label = (i + 1 < node_labels.size()) ? node_labels[i + 1] : "";

        std::string value_ptr_label;

        if (auto* num_lit = dynamic_cast<NumberLiteral*>(expr.get())) {
            if (num_lit->literal_type == NumberLiteral::LiteralType::Integer) {
                node_data.type_tag = 1; // ATOM_INT
                node_data.value_bits = static_cast<uint64_t>(num_lit->int_value);
                node_data.value_is_ptr = false;
            } else {
                node_data.type_tag = 2; // ATOM_FLOAT
                double f_val = num_lit->float_value;
                memcpy(&node_data.value_bits, &f_val, sizeof(double));
                node_data.value_is_ptr = false;
            }
        } else if (auto* str_lit = dynamic_cast<StringLiteral*>(expr.get())) {
            node_data.type_tag = 3; // ATOM_STRING
            node_data.value_ptr_label = add_string_literal(str_lit->value);
            node_data.value_is_ptr = true;
        } else if (auto* list_lit = dynamic_cast<ListExpression*>(expr.get())) {
            node_data.type_tag = 4; // ATOM_LIST_POINTER
            node_data.value_ptr_label = add_list_literal(list_lit);
            node_data.value_is_ptr = true;
        } else {
            throw std::runtime_error("List initializers must be constant literals.");
        }
        list_info.nodes.push_back(node_data);
    }

    // Memoize the label for this list literal
    list_literal_label_map[canonical_key] = list_info.header_label;
    list_literals_.push_back(list_info);

    return list_info.header_label;
}


std::string DataGenerator::add_float_table_literal(const std::vector<ExprPtr>& initializers) {
    std::string label = "L_ftbl" + std::to_string(next_float_table_id_++);
    std::vector<double> values;
    for (const auto& expr : initializers) {
        if (auto* num_lit = dynamic_cast<NumberLiteral*>(expr.get())) {
            values.push_back(num_lit->literal_type == NumberLiteral::LiteralType::Float ? num_lit->float_value : static_cast<double>(num_lit->int_value));
        } else {
            throw std::runtime_error("FTABLE initializers must be constant numeric literals.");
        }
    }
    float_table_literals_.push_back({label, values});
    return label;
}

void DataGenerator::add_global_variable(const std::string& name, ExprPtr initializer) {
    static_variables_.push_back({name, std::move(initializer)});
}

// ** REFACTORED `generate_rodata_section` **
void DataGenerator::generate_rodata_section(InstructionStream& stream) {
    // Emit String Literals (no changes)
    for (const auto& info : string_literals_) {
        stream.add(Instruction::as_label(info.label, SegmentType::RODATA));
        stream.add_data64(info.value.length() - 2, "", SegmentType::RODATA);
        for (char32_t ch : info.value) {
            stream.add_data32(static_cast<uint32_t>(ch), "", SegmentType::RODATA);
        }
        stream.add_data_padding(8);
    }
    // ... (emit float, table, ftable literals as before) ...
    for (const auto& info : float_literals_) {
        stream.add(Instruction::as_label(info.label, SegmentType::RODATA));
        uint64_t float_bits;
        std::memcpy(&float_bits, &info.value, sizeof(uint64_t));
        stream.add_data64(float_bits, "", SegmentType::RODATA);
    }

    for (const auto& table : table_literals_) {
        stream.add(Instruction::as_label(table.label, SegmentType::RODATA));
        stream.add_data64(table.values.size(), "", SegmentType::RODATA);
        for (auto v : table.values) {
            stream.add_data64(static_cast<uint64_t>(v), "", SegmentType::RODATA);
        }
    }

    for (const auto& ftable : float_table_literals_) {
        stream.add(Instruction::as_label(ftable.label, SegmentType::RODATA));
        stream.add_data64(ftable.values.size(), "", SegmentType::RODATA);
        for (auto v : ftable.values) {
            uint64_t float_bits;
            std::memcpy(&float_bits, &v, sizeof(uint64_t));
            stream.add_data64(float_bits, "", SegmentType::RODATA);
        }
    }

    // --- ** CORRECTED LIST EMISSION LOGIC ** ---

    // Tracing setup (moved outside the main loop)
    if (enable_tracing_) {
        std::cout << "[DataGenerator TRACE] Entering generate_rodata_section." << std::endl;
        std::cout << "[DataGenerator TRACE] Size of list_literals_ vector is: " << list_literals_.size() << std::endl;
    }

    // Main loop for emission and detailed tracing
    for (const auto& list_info : list_literals_) {
        // --- Detailed Trace ---
        if (enable_tracing_) {
            std::cout << "[DataGenerator TRACE] >> Processing header: " << list_info.header_label << std::endl;
            std::cout << "[DataGenerator TRACE]   Internal nodes vector size: " << list_info.nodes.size() << std::endl;
            std::cout << "[DataGenerator TRACE]   --- Dumping Node Labels ---" << std::endl;
            for (const auto& node_data : list_info.nodes) {
                std::cout << "[DataGenerator TRACE]     -> " << node_data.node_label << std::endl;
            }
            std::cout << "[DataGenerator TRACE]   --- End Dump ---" << std::endl;
        }
        // --- End Detailed Trace ---

        // 1. Emit Header (32 bytes, compatible with ListLiteralHeader)
        // FIX: Define the label ONCE (removed the duplicate emission)
        stream.add(Instruction::as_label(list_info.header_label, SegmentType::RODATA));

        // --- ListLiteralHeader emission (matches runtime/ListDataTypes.h) ---
        // type (offset 0) and padding (offset 4)
        stream.add_data32(ATOM_SENTINEL, "", SegmentType::RODATA); // type = ATOM_SENTINEL (0)
        stream.add_data32(0, "", SegmentType::RODATA); // padding

        // value (tail pointer) at offset 8
        // tail pointer (offset 8)
        if (!list_info.nodes.empty()) {
            emit_absolute_pointer(stream, list_info.nodes.back().node_label, SegmentType::RODATA);
        } else {
            stream.add_data64(0, "", SegmentType::RODATA); // NULL tail
        }

        // next (head pointer) at offset 16
        // head pointer (offset 16)
        if (!list_info.nodes.empty()) {
            emit_absolute_pointer(stream, list_info.nodes.front().node_label, SegmentType::RODATA);
        } else {
            stream.add_data64(0, "", SegmentType::RODATA); // NULL head
        }

        // length (offset 24)
        stream.add_data64(list_info.length, "", SegmentType::RODATA);

        // 2. Emit Nodes
        for (const auto& node_data : list_info.nodes) {
            stream.add(Instruction::as_label(node_data.node_label, SegmentType::RODATA));
            stream.add_data32(node_data.type_tag, "", SegmentType::RODATA);
            stream.add_data32(0, "", SegmentType::RODATA); // padding

            // Value
            if (node_data.value_is_ptr) {
                emit_absolute_pointer(stream, node_data.value_ptr_label, SegmentType::RODATA);
            } else {
                stream.add_data64(node_data.value_bits, "", SegmentType::RODATA);
            }

            // Next Pointer
            if (!node_data.next_node_label.empty()) {
                emit_absolute_pointer(stream, node_data.next_node_label, SegmentType::RODATA);
            } else {
                stream.add_data64(0, "", SegmentType::RODATA);
            }
        }
    }
}


// Other DataGenerator methods (calculate_global_offsets, generate_data_section, etc.) remain the same.
void DataGenerator::calculate_global_offsets() {
    global_word_offsets_.clear();
    size_t current_word_offset = 0;
    for (const auto& info : static_variables_) {
        global_word_offsets_[info.label] = current_word_offset;
        if (symbol_table_) {
            symbol_table_->setSymbolDataLocation(info.label, current_word_offset);
        }
        current_word_offset++;
    }
}

/**
 * @brief Generates a human-readable listing of the .rodata section for debugging.
 * This function iterates through all registered read-only literals (strings, floats, lists)
 * and creates a formatted string showing their labels and content.
 *
 * @param label_manager A const reference to the label manager to resolve addresses.
 * @return A std::string containing the formatted .rodata listing.
 */


/**
 * @brief Generates a human-readable string for a single list literal.
 * This is a debugging helper that formats the contents of a ListLiteralInfo
 * struct, making it easy to inspect the structure of a statically generated list.
 *
 * @param list_info The list literal structure to display.
 * @return A std::string containing the formatted list information.
 */
std::string DataGenerator::display_literal_list(const ListLiteralInfo& list_info) const {
    std::stringstream ss;
    ss << "\n--- Displaying List Literal: " << list_info.header_label << " ---\n";

    // --- Display Header ---
    ss << list_info.header_label << ":\n";
    ss << "  .long " << ATOM_SENTINEL << "  ; Type Tag (ATOM_SENTINEL)\n";
    ss << "  .long 0          ; Padding\n";
    ss << "  .quad " << (list_info.nodes.empty() ? "0" : list_info.nodes.back().node_label) << "  ; Tail Pointer\n";
    ss << "  .quad " << (list_info.nodes.empty() ? "0" : list_info.nodes.front().node_label) << "  ; Head Pointer\n";
    ss << "  .quad " << list_info.length << "  ; Length\n";

    // --- Display Nodes ---
    for (const auto& node_data : list_info.nodes) {
        ss << node_data.node_label << ":\n";
        ss << "  .long " << node_data.type_tag << "  ; Type Tag\n";
        ss << "  .long 0          ; Padding\n";
        if (node_data.value_is_ptr) {
            ss << "  .quad " << node_data.value_ptr_label << "  ; Value (Pointer)\n";
        } else {
            // Interpret the raw bits for better readability
            if (node_data.type_tag == 2 /* ATOM_FLOAT */) {
                double f_val;
                memcpy(&f_val, &node_data.value_bits, sizeof(double));
                ss << "  .double " << std::fixed << std::setprecision(6) << f_val << "  ; Value (Immediate Float)\n";
            } else { // Assumes ATOM_INT or other integer-like types
                ss << "  .quad " << static_cast<int64_t>(node_data.value_bits) << "  ; Value (Immediate Int)\n";
            }
        }
        ss << "  .quad " << (node_data.next_node_label.empty() ? "0" : node_data.next_node_label) << "  ; Next Pointer\n";
    }
    
    ss << "-------------------------------------\n";
    return ss.str();
}

size_t DataGenerator::get_global_word_offset(const std::string& name) const {
    auto it = global_word_offsets_.find(name);
    if (it == global_word_offsets_.end()) {
        throw std::runtime_error("Global variable '" + name + "' has no calculated offset.");
    }
    return it->second;
}

bool DataGenerator::is_global_variable(const std::string& name) const {
    return std::any_of(static_variables_.begin(), static_variables_.end(),
                       [&](const auto& var) { return var.label == name; });
}

void DataGenerator::generate_data_section(InstructionStream& stream) {
    calculate_global_offsets();
    Instruction label_instr;
    label_instr.is_label_definition = true;
    label_instr.target_label = "L__data_segment_base";
    label_instr.segment = SegmentType::DATA;
    stream.add(label_instr);

    if (static_variables_.empty()) {
        return;
    }

    for (const auto& info : static_variables_) {
        NumberLiteral* num_lit = dynamic_cast<NumberLiteral*>(info.initializer.get());
        long initial_value = num_lit ? num_lit->int_value : 0;
        stream.add_data64(static_cast<uint64_t>(initial_value), "", SegmentType::DATA);
    }
}

std::string DataGenerator::generate_rodata_listing(const LabelManager& label_manager) {
    // Implementation remains the same
    return "";
}

void DataGenerator::populate_data_segment(void* data_base_address, const LabelManager& label_manager) {
    // Implementation remains the same
}

std::string DataGenerator::generate_data_listing(const LabelManager& label_manager, void* data_base_address) {
    // Implementation remains the same
    return "";
}
