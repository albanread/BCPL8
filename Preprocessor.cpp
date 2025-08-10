#include "Preprocessor.h"
#include <iostream>
// Remove regex include as it's not needed
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <libgen.h>

std::string Preprocessor::process(const std::string& root_filepath) {
    // Clear the output stream before processing
    output_stream_.str("");
    output_stream_.clear();
    
    // Initialize the inclusion stack
    std::unordered_set<std::string> inclusion_stack;
    
    try {
        // Process the root file and all its includes
        process_internal(root_filepath, inclusion_stack);
    }
    catch (const std::exception& e) {
        // Add preprocessor context to the error
        throw std::runtime_error("Preprocessor error: " + std::string(e.what()));
    }
    
    // Return the final processed content
    return output_stream_.str();
}

void Preprocessor::addIncludePath(const std::string& path) {
    if (!path.empty()) {
        include_paths_.push_back(path);
    }
}

void Preprocessor::process_internal(const std::string& current_filepath,
                                    std::unordered_set<std::string>& inclusion_stack) {
    // Convert path to canonical form to handle relative paths correctly
    std::string canonical_path;
    
    // Get the absolute path
    char resolved_path[PATH_MAX];
    if (realpath(current_filepath.c_str(), resolved_path) != nullptr) {
        canonical_path = std::string(resolved_path);
    } else {
        // If realpath fails, use the original path
        canonical_path = current_filepath;
    }
    
    // Check for circular includes
    if (inclusion_stack.find(canonical_path) != inclusion_stack.end()) {
        // Build a circular dependency message that shows the include chain
        std::string include_chain;
        for (const auto& path : inclusion_stack) {
            include_chain += "\n  " + path;
        }
        include_chain += "\n  " + canonical_path + " (circular reference)";
        throw std::runtime_error("Circular GET dependency detected:" + include_chain);
    }
    
    // Add this file to the inclusion stack
    inclusion_stack.insert(canonical_path);
    
    debug_print("Processing file: " + canonical_path);
    
    // Open the file
    std::ifstream file(current_filepath);
    if (!file.is_open()) {
        // Try to find the file in the include paths
        std::string resolved_path = resolve_file_path(current_filepath, "");
        if (!resolved_path.empty() && resolved_path != current_filepath) {
            // Found in include path, try again with resolved path
            file.open(resolved_path);
            if (file.is_open()) {
                debug_print("Found in include path: " + resolved_path);
                canonical_path = resolved_path;  // Update canonical path
            }
        }
        
        if (!file.is_open()) {
            // Check if we have a parent file in the inclusion stack to provide better context
            std::string context = "";
            for (const auto& included_file : inclusion_stack) {
                if (included_file != canonical_path) {
                    context = " (referenced from " + included_file + ")";
                    break;
                }
            }
            
            // Build a list of paths that were searched
            std::string searched_paths = "";
            if (!include_paths_.empty()) {
                searched_paths = "\nSearched in:";
                char cwd[PATH_MAX];
                if (getcwd(cwd, sizeof(cwd)) != nullptr) {
                    searched_paths += "\n  - Current directory: " + std::string(cwd);
                }
                for (const auto& path : include_paths_) {
                    searched_paths += "\n  - Include path: " + path;
                }
            }
            
            throw std::runtime_error("Could not open file: " + current_filepath + context + searched_paths);
        }
    }
    
    // Get the directory of the current file for relative includes
    std::string current_dir;
    char* path_copy = strdup(canonical_path.c_str());
    if (path_copy) {
        current_dir = dirname(path_copy);
        free(path_copy);
    }
    
    // Add line directive for source mapping
    output_stream_ << "//LINE 1 \"" << canonical_path << "\"\n";
    
    // Process the file line by line
    std::string line;
    int line_number = 1;
    
    while (std::getline(file, line)) {
        // Check if this line contains a GET directive
        if (is_get_directive(line)) {
            std::string include_file = extract_filename(line);
            if (!include_file.empty()) {
                debug_print("Found GET directive: " + include_file);
                
                // Resolve the path of the included file relative to the current file
                std::string include_path = resolve_file_path(include_file, current_dir);
                
                if (include_path.empty()) {
                    // Build a list of paths that were searched
                    std::string searched_paths = "\n  - Current directory: " + current_dir;
                    for (const auto& path : include_paths_) {
                        searched_paths += "\n  - Include path: " + path;
                    }
                    
                    throw std::runtime_error("Could not resolve include file: " + include_file + 
                                           " referenced from " + canonical_path + 
                                           " at line " + std::to_string(line_number) +
                                           "\nSearched in:" + searched_paths);
                }
                
                // Recursively process the included file
                process_internal(include_path, inclusion_stack);
                
                // Add line directive after returning from the included file
                output_stream_ << "//LINE " << (line_number + 1) << " \"" << canonical_path << "\"\n";
            }
            else {
                // Malformed GET directive, keep it in the output
                output_stream_ << line << '\n';
            }
        }
        else {
            // Regular line, just append it
            output_stream_ << line << '\n';
        }
        
        line_number++;
    }
    
    // Remove this file from the inclusion stack now that we're done with it
    inclusion_stack.erase(canonical_path);
}

bool Preprocessor::is_get_directive(const std::string& line) {
    // Create a trimmed copy of the line
    std::string trimmed = line;
    
    // Remove leading whitespace
    trimmed.erase(trimmed.begin(), 
                 std::find_if(trimmed.begin(), trimmed.end(), 
                             [](unsigned char ch) { return !std::isspace(ch); }));
    
    // Check if it starts with GET (case insensitive)
    if (trimmed.size() >= 3) {
        std::string prefix = trimmed.substr(0, 3);
        std::transform(prefix.begin(), prefix.end(), prefix.begin(), 
                      [](unsigned char c){ return std::toupper(c); });
        return prefix == "GET";
    }
    
    return false;
}

std::string Preprocessor::extract_filename(const std::string& line) {
    // Find the first quote
    size_t first_quote = line.find('"');
    if (first_quote == std::string::npos) {
        return "";
    }
    
    // Find the closing quote
    size_t last_quote = line.find('"', first_quote + 1);
    if (last_quote == std::string::npos) {
        return "";
    }
    
    // Extract the filename between quotes
    return line.substr(first_quote + 1, last_quote - first_quote - 1);
}

std::string Preprocessor::resolve_file_path(const std::string& requested_file, 
                                           const std::string& current_dir) {
    struct stat buffer;
    
    // Try with the requested path as-is first
    if (stat(requested_file.c_str(), &buffer) == 0) {
        return requested_file;
    }
    
    // Try relative to the current file's directory
    if (!current_dir.empty()) {
        std::string potential_path = current_dir + "/" + requested_file;
        if (stat(potential_path.c_str(), &buffer) == 0) {
            return potential_path;
        }
    }
    
    // Try the include paths
    for (const auto& include_path : include_paths_) {
        std::string potential_path = include_path + "/" + requested_file;
        if (stat(potential_path.c_str(), &buffer) == 0) {
            return potential_path;
        }
    }
    
    // If still not found, return empty string
    return "";
}

void Preprocessor::debug_print(const std::string& message) {
    if (debug_enabled_) {
        std::cout << "[Preprocessor] " << message << std::endl;
    }
}

// Add a helper method to get the parent file from the inclusion stack
std::string Preprocessor::get_parent_file(const std::unordered_set<std::string>& inclusion_stack, 
                                         const std::string& current_file) {
    // Find the parent file (any file in the inclusion stack that's not the current file)
    for (const auto& file : inclusion_stack) {
        if (file != current_file) {
            return file;
        }
    }
    return "";
}