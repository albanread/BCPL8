#ifndef DATA_GENERATOR_H
#define DATA_GENERATOR_H

#include "AST.h"
#include "DataTypes.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

class InstructionStream;
class LabelManager; // Forward declaration

class DataGenerator {
public:
    // Struct to hold information about static variables.
    struct StaticVariableInfo {
        std::string label;
        ExprPtr initializer;
    };

    // Struct to hold information about string literals.
    struct StringLiteralInfo {
        std::string label;
        std::u32string value;
    };

    // Struct to hold information about float literals.
    struct FloatLiteralInfo {
        std::string label;
        double value;
    };

    explicit DataGenerator(bool enable_tracing = false);

    // Public methods for adding data
    std::string add_string_literal(const std::string& value);
    std::string add_float_literal(double value);
    void add_global_variable(const std::string& name, ExprPtr initializer);

    // Public methods for generating sections
    void generate_rodata_section(InstructionStream& stream);
    void generate_data_section(InstructionStream& stream);

    // Public methods for analysis and code generation support
    void calculate_global_offsets();
    size_t get_global_word_offset(const std::string& name) const;
    bool is_global_variable(const std::string& name) const;
    
    // Public methods for debug listings
    std::string generate_rodata_listing(const LabelManager& label_manager) const;
    std::string generate_data_listing(const LabelManager& label_manager, void* data_base_address) const;

    // Populates the JIT data segment with initial values for global variables.
    void populate_data_segment(void* data_base_address, const LabelManager& label_manager) const;

private:
    void define_data_segment_base(InstructionStream& stream);

    // --- ADDED: All missing member variables ---
    bool enable_tracing_;
    size_t next_string_id_;
    size_t next_float_id_;

    std::vector<StringLiteralInfo> string_literals_;
    std::vector<FloatLiteralInfo> float_literals_;
    std::vector<StaticVariableInfo> static_variables_;

public:
    const std::vector<StringLiteralInfo>& get_string_literals() const { return string_literals_; }
    const std::vector<FloatLiteralInfo>& get_float_literals() const { return float_literals_; }

    std::unordered_map<std::string, std::string> string_literal_map_;
    std::unordered_map<double, std::string> float_literal_map_;
    std::unordered_map<std::string, size_t> global_word_offsets_;
};

#endif // DATA_GENERATOR_H