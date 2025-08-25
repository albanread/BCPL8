#include "CallFrameManager.h"
#include <sstream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <string>

#include <sstream>
#include "DataTypes.h"

// Bitfield-aware VarType display utility
static std::string var_type_to_string(VarType t) {
    int64_t v = static_cast<int64_t>(t);
    if (v == 0) return "UNKNOWN";

    std::string result;
    if (v & static_cast<int64_t>(VarType::CONST)) result += "CONST|";
    if (v & static_cast<int64_t>(VarType::POINTER_TO)) result += "POINTER_TO|";
    if (v & static_cast<int64_t>(VarType::LIST)) result += "LIST|";
    if (v & static_cast<int64_t>(VarType::VEC)) result += "VEC|";
    if (v & static_cast<int64_t>(VarType::TABLE)) result += "TABLE|";
    
    // Handle base types
    if (v & static_cast<int64_t>(VarType::INTEGER)) result += "INTEGER|";
    if (v & static_cast<int64_t>(VarType::FLOAT)) result += "FLOAT|";
    if (v & static_cast<int64_t>(VarType::STRING)) result += "STRING|";
    if (v & static_cast<int64_t>(VarType::ANY)) result += "ANY|";

    // Remove the trailing '|'
    if (!result.empty() && result.back() == '|') {
        result.pop_back();
    }
    
    return result;
}

std::string CallFrameManager::display_frame_layout() const {
    if (!is_prologue_generated && function_name.empty()) {
        return "Call Frame Layout: Not yet configured/finalized.\n";
    }

    std::stringstream ss;
    ss << "--- Call Frame Layout for function: " << function_name << " (Total Size: " << final_frame_size << " bytes) ---\n";
    ss << std::left << std::setw(9) << "Offset" << "| " << std::setw(38) << "Content" << "| Type\n";
    ss << "------------------------------------------------------\n";

    ss << std::setw(9) << "+0" << "| Old Frame Pointer (x29)     <-- FP (x29) points here\n";
    ss << std::setw(9) << "+8" << "| Return Address (Caller's PC)\n";
    
    // Only show canaries in frame layout if they're enabled
    if (enable_stack_canaries) {
        ss << std::setw(9) << "+16" << "| Upper Stack Canary (0x" << std::hex << UPPER_CANARY_VALUE << std::dec << ")\n";
        ss << std::setw(9) << "+" << (16 + CANARY_SIZE) << "| Lower Stack Canary (0x" << std::hex << LOWER_CANARY_VALUE << std::dec << ")\n";
    }

    // Use a struct to hold item details for sorting and printing
    struct FrameItem {
        std::string description;
        int offset;
        std::string type;
    };
    std::vector<FrameItem> items_to_display;

    // Add local variables with their types
    for (const auto& decl : local_declarations) {
        if (variable_offsets.count(decl.name)) {
            VarType type = get_variable_type(decl.name);
            items_to_display.push_back({
                "Local: " + decl.name,
                variable_offsets.at(decl.name),
                var_type_to_string(type)
            });
        }
    }

    // Add saved registers with their types
    for (const auto& reg_name : callee_saved_registers_to_save) {
         if (variable_offsets.count(reg_name)) {
            VarType type = get_variable_type(reg_name);
            items_to_display.push_back({
                "Saved Reg: " + reg_name,
                variable_offsets.at(reg_name),
                var_type_to_string(type)
            });
        }
    }

    // Add spill slots to display
    for (const auto& pair : spill_variable_offsets_) {
        VarType type = get_variable_type(pair.first);
        items_to_display.push_back({
            "Spill Slot: " + pair.first,
            pair.second,
            var_type_to_string(type)
        });
    }

    // Sort items by their stack offset for a clean, top-down view
    std::sort(items_to_display.begin(), items_to_display.end(), [](const auto& a, const auto& b) {
        return a.offset < b.offset;
    });

    // Print all sorted items
    for (const auto& item : items_to_display) {
        ss << std::left << std::setw(9) << ("+" + std::to_string(item.offset))
           << "| " << std::setw(38) << item.description
           << "| " << item.type << "\n";
    }

    ss << "------------------------------------------------------\n";
    ss << "                                     <-- SP (+" << final_frame_size << " from FP)\n";

    return ss.str();
}
