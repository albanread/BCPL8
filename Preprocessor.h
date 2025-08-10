#ifndef PREPROCESSOR_H
#define PREPROCESSOR_H

#include <string>
#include <sstream>
#include <limits.h>
#include <fstream>
#include <unordered_set>
#include <stdexcept>
#include <algorithm>
#include <cctype>
#include <vector>

class Preprocessor {
public:
    // Constructor
    Preprocessor() = default;

    // Main entry point to process the root source file
    std::string process(const std::string& root_filepath);

    // Add an include search path
    void addIncludePath(const std::string& path);

    // Enable debug output for the preprocessor
    void enableDebug(bool enable = true) { debug_enabled_ = enable; }

private:
    // Recursive helper to process a file and its includes
    void process_internal(const std::string& current_filepath, 
                          std::unordered_set<std::string>& inclusion_stack);

    // Helper to extract the filename from a GET directive line
    std::string extract_filename(const std::string& line);

    // Resolve file path using include directories
    std::string resolve_file_path(const std::string& requested_file, 
                                 const std::string& current_dir);
    
    // Check if a line contains a GET directive
    bool is_get_directive(const std::string& line);

    // Print debug information if debug mode is enabled
    void debug_print(const std::string& message);

    // Helper to get the parent file from the inclusion stack
    std::string get_parent_file(const std::unordered_set<std::string>& inclusion_stack,
                               const std::string& current_file);

    // The stream that accumulates the final source code
    std::ostringstream output_stream_;

    // List of include search directories
    std::vector<std::string> include_paths_;

    // Flag for enabling debug output
    bool debug_enabled_ = false;
};

#endif // PREPROCESSOR_H