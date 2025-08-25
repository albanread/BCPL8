#pragma once
#include "LiveInterval.h"
#include <map>
#include <vector>
#include <string>

class ControlFlowGraph;

// LiveIntervalPass computes live intervals for all variables in a function's CFG.
// It should be run after liveness analysis and before register allocation.
class LiveIntervalPass {
public:
    // Run the pass on a function's CFG. Populates internal interval data.
    void run(const ControlFlowGraph& cfg, const std::string& functionName);

    // Retrieve the computed intervals for a function.
    const std::vector<LiveInterval>& getIntervalsFor(const std::string& functionName) const;

private:
    // Map from function name to its vector of live intervals.
    std::map<std::string, std::vector<LiveInterval>> function_intervals_;
};