#include "LivenessAnalysisPass.h"

const std::set<std::string>& LivenessAnalysisPass::get_in_set(BasicBlock* block) const {
    auto it = in_sets_.find(block);
    return (it != in_sets_.end()) ? it->second : empty_set_;
}