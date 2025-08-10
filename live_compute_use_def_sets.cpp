#include "LivenessAnalysisPass.h"
#include <iostream>

void LivenessAnalysisPass::compute_use_def_sets() {
    if (trace_enabled_) {
        std::cout << "[LivenessAnalysisPass] Entering compute_use_def_sets()" << std::endl;
    }
    size_t cfg_count = 0;
    for (const auto& pair : cfgs_) {
        ++cfg_count;
        if (!pair.second) {
            if (trace_enabled_) {
                std::cout << "[LivenessAnalysisPass] Warning: CFG for function '" << pair.first << "' is null." << std::endl;
            }
            continue;
        }
        if (trace_enabled_) {
            std::cout << "[LivenessAnalysisPass] CFG #" << cfg_count << " for function '" << pair.first << "' has " << pair.second->blocks.size() << " blocks." << std::endl;
        }
        size_t block_count = 0;
        for (const auto& block_pair : pair.second->blocks) {
            ++block_count;
            if (!block_pair.second) {
                if (trace_enabled_) {
                    std::cout << "[LivenessAnalysisPass] Warning: BasicBlock #" << block_count << " in function '" << pair.first << "' is null." << std::endl;
                }
                continue;
            }
            if (trace_enabled_) {
                std::cout << "[LivenessAnalysisPass] Analyzing block #" << block_count << " (id=" << block_pair.second->id << ") in function '" << pair.first << "'" << std::endl;
            }
            try {
                analyze_block(block_pair.second.get());
            } catch (const std::exception& ex) {
                std::cerr << "[LivenessAnalysisPass] Exception in analyze_block for block " << block_pair.second->id << ": " << ex.what() << std::endl;
                throw;
            } catch (...) {
                std::cerr << "[LivenessAnalysisPass] Unknown exception in analyze_block for block " << block_pair.second->id << std::endl;
                throw;
            }
        }
    }
    if (trace_enabled_) {
        std::cout << "[LivenessAnalysisPass] Exiting compute_use_def_sets()" << std::endl;
    }
}