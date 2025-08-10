#include "LivenessAnalysisPass.h"

const std::set<std::string>& LivenessAnalysisPass::get_out_set(BasicBlock* block) const {
    auto it = out_sets_.find(block);
    return (it != out_sets_.end()) ? it->second : empty_set_;
}