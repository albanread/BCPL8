#ifndef LABEL_MANAGER_H
#define LABEL_MANAGER_H

#include "RuntimeManager.h"

#include <string>
#include <unordered_map>
#include <stdexcept>

// The LabelManager creates and defines labels, mapping their names to addresses.
// Singleton pattern: only one instance exists.
class LabelManager {
public:
    // Singleton accessor
    static LabelManager& instance() {
        static LabelManager instance;
        return instance;
    }

    // Checks if a label corresponds to a runtime routine
    bool is_runtime_label(const std::string& label_name) const;



    // Creates a new unique label name.
    std::string create_label();

    // --- Helper methods for codegen compatibility ---
    // Returns a global label for a variable name (e.g., "global_varname")
    std::string create_global_label(const std::string& name) const;
    // Returns a static label for a variable name (e.g., "static_varname")
    std::string create_static_label(const std::string& name);
    // Returns the label for a variable name (for now, just returns the name)
    std::string get_label(const std::string& name) const;
    // Returns the next label id (for unique label generation)
    size_t get_next_id() const;

    // Defines a label, associating it with a specific address (offset in bytes).
    void define_label(const std::string& label_name, size_t address);

    // Retrieves the address associated with a label.
    size_t get_label_address(const std::string& label_name) const;

    // Checks if a label is defined.
    bool is_label_defined(const std::string& label_name) const;

    // Returns a map of all defined labels and their addresses.
    const std::unordered_map<std::string, size_t>& get_defined_labels() const { return defined_labels_; }

private:
    // Private constructor for singleton
    LabelManager();
    // Delete copy/move to enforce singleton
    LabelManager(const LabelManager&) = delete;
    LabelManager& operator=(const LabelManager&) = delete;
    LabelManager(LabelManager&&) = delete;
    LabelManager& operator=(LabelManager&&) = delete;

    std::unordered_map<std::string, size_t> defined_labels_;
    size_t next_label_id_;

    // List of runtime routine labels for identification
    const std::unordered_map<std::string, bool> runtime_labels_ = {
        {"runtime_start", true},
        {"runtime_end", true},
        {"runtime_handler", true}
    };
};

#endif // LABEL_MANAGER_H
