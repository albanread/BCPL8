#include "LabelManager.h"
#include <stdexcept>
#include "RuntimeManager.h"

LabelManager::LabelManager() : next_label_id_(0) {
    // Initialize runtime labels if needed (currently predefined in header)
}

// No public constructor: singleton pattern

std::string LabelManager::create_label() {
    return ".L" + std::to_string(next_label_id_++);
}

// --- Helper methods for codegen compatibility ---

// Returns a global label for a variable name (e.g., "global_varname")
std::string LabelManager::create_global_label(const std::string& name) const {
    return "global_" + name;
}

// Returns a static label for a variable name (e.g., "static_varname")
std::string LabelManager::create_static_label(const std::string& name) {
    return "static_" + name;
}

// Returns the label for a variable name (for now, just returns the name)
std::string LabelManager::get_label(const std::string& name) const {
    return name;
}

// Returns the next label id (for unique label generation)
size_t LabelManager::get_next_id() const {
    return next_label_id_;
}

bool LabelManager::is_runtime_label(const std::string& label_name) const {
    return RuntimeManager::instance().is_function_registered(label_name);
}

void LabelManager::define_label(const std::string& label_name, size_t address) {
    if (is_label_defined(label_name)) {
        throw std::runtime_error("Error: Label '" + label_name + "' already defined.");
    }
    defined_labels_[label_name] = address;
}

size_t LabelManager::get_label_address(const std::string& label_name) const {
    auto it = defined_labels_.find(label_name);
    if (it == defined_labels_.end()) {
        throw std::runtime_error("Error: Label '" + label_name + "' not defined.");
    }
    return it->second;
}

bool LabelManager::is_label_defined(const std::string& label_name) const {
    return defined_labels_.count(label_name) > 0;
}

void LabelManager::reset() {
    defined_labels_.clear();
    next_label_id_ = 0;
}


