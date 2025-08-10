#ifndef INSTRUCTION_STREAM_H
#define INSTRUCTION_STREAM_H

#include "Encoder.h" // For Instruction struct
#include "LabelManager.h" // For LabelManager class
#include <vector>
#include <memory>

// The InstructionStream holds a sequence of Instruction objects.
// It also interacts with the LabelManager to define labels at specific points.
class InstructionStream {
public:
    // Constructor takes a reference to the LabelManager
    InstructionStream(LabelManager& label_manager);

    // Adds an instruction to the stream.
    void add(const Instruction& instr);

    // Creates and defines a new label at the current end of the stream.
    // Returns the name of the created label.
    std::string create_and_define_label();

    // Defines an existing label at the current end of the stream.
    void define_label(const std::string& label_name);

    void add_padcode(int alignment);
    // Adds a comment to the instruction stream (for debugging/documentation purposes).
    void add_comment(const std::string& comment);
    // Pads data sections with 32-bit zeros to a given byte alignment.
    void add_data_padding(int alignment);
    void add_data8(uint8_t data, const std::string& label_name = "");
    void add_data16(uint16_t data, const std::string& label_name = "");
    void add_data32(uint32_t data, const std::string& label_name = "", SegmentType segment = SegmentType::DATA);
    void add_data64(uint64_t data, const std::string& label_name = "", SegmentType segment = SegmentType::DATA);

    // New function to add zero-filled 32-bit data for padding
    void add_zero_data32(int count);

    // Returns the current size of the stream in instructions (not bytes).
    size_t size() const { return instructions_.size(); }

    // Returns an estimation of the current address in bytes for branch distance calculations.
    // This considers instruction sizes (4 bytes each) and data sizes, ignoring labels.
    size_t get_current_address() const;

    // Provides access to the underlying vector of instructions (const reference).
    const std::vector<Instruction>& get_instructions_ref() const { return instructions_; }

    /**
     * @brief Returns a copy of the current instructions in the stream.
     * This allows external classes like the optimizer to work on the data.
     */
    std::vector<Instruction> get_instructions() const;

    /**
     * @brief Replaces the entire instruction stream with a new set of instructions.
     * This is used by the optimizer to commit its changes.
     */
    void replace_instructions(const std::vector<Instruction>& new_instructions);

    // Adds a vector of data instructions to the stream and defines their labels.
    void add_data_instructions(const std::vector<Instruction>& data_instructions);

private:
    std::vector<Instruction> instructions_;
    LabelManager& label_manager_;
};

#endif // INSTRUCTION_STREAM_H
