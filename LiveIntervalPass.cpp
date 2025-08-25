#include "LiveIntervalPass.h"
#include "LiveInterval.h"
#include "CFGBuilderPass.h"
#include "ControlFlowGraph.h"
#include <map>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <stdexcept>
#include <iostream>

// This pass computes live intervals for all variables in a function.
// It should be run after liveness analysis and before register allocation.

void LiveIntervalPass::run(const ControlFlowGraph& cfg, const std::string& functionName) {
    // Map from variable name to its live interval
    std::map<std::string, LiveInterval> intervals;

    // 1. Assign instruction numbers to each statement in codegen order.
    // We'll use a vector of pointers to statements for deterministic order.
    std::vector<Statement*> ordered_statements;
    int instruction_number = 0;

    // Deterministic order: sorted block names
    std::vector<std::string> block_names;
    for (const auto& pair : cfg.get_blocks()) {
        block_names.push_back(pair.first);
    }
    std::sort(block_names.begin(), block_names.end());

    for (const auto& block_name : block_names) {
        const auto& block = cfg.get_blocks().at(block_name);
        for (const auto& stmt_ptr : block->statements) {
            ordered_statements.push_back(stmt_ptr.get());
        }
    }

    // 2. Walk through statements, record first and last use/def for each variable.
    for (Statement* stmt : ordered_statements) {
        // For simplicity, assume stmt->get_used_variables() and stmt->get_defined_variables() exist.
        // These should return std::vector<std::string> of variable names.
        std::vector<std::string> used_vars = stmt->get_used_variables();
        std::vector<std::string> def_vars = stmt->get_defined_variables();

        for (const auto& var : used_vars) {
            if (intervals.find(var) == intervals.end()) {
                intervals[var] = LiveInterval(var, instruction_number, instruction_number);
            }
            intervals[var].end_point = instruction_number;
        }
        for (const auto& var : def_vars) {
            if (intervals.find(var) == intervals.end()) {
                intervals[var] = LiveInterval(var, instruction_number, instruction_number);
            }
            intervals[var].end_point = instruction_number;
        }
        instruction_number++;
    }

    // 3. Store the intervals for this function.
    std::vector<LiveInterval> result;
    for (const auto& pair : intervals) {
        result.push_back(pair.second);
    }
    function_intervals_[functionName] = std::move(result);
}

const std::vector<LiveInterval>& LiveIntervalPass::getIntervalsFor(const std::string& functionName) const {
    static const std::vector<LiveInterval> empty;
    auto it = function_intervals_.find(functionName);
    return it != function_intervals_.end() ? it->second : empty;
}