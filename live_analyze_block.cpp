#include "LivenessAnalysisPass.h"
#include <iostream>

void LivenessAnalysisPass::analyze_block(BasicBlock* block) {
    if (trace_enabled_) {
        std::cout << "[LivenessAnalysisPass] Entering analyze_block for block: " 
                  << (block ? block->id : "nullptr") << std::endl;
    }

    if (!block) {
        if (trace_enabled_) {
            std::cerr << "[LivenessAnalysisPass] ERROR: analyze_block called with nullptr block!" << std::endl;
        }
        return;
    }

    current_use_set_.clear();
    current_def_set_.clear();

    // Iterate through all statements in the block
    int stmt_idx = 0;
    for (const auto& stmt : block->statements) {
        if (!stmt) {
            if (trace_enabled_) {
                std::cout << "[LivenessAnalysisPass] Warning: Null statement at index " << stmt_idx 
                          << " in block " << block->id << std::endl;
            }
            ++stmt_idx;
            continue;
        }
        try {
            stmt->accept(*this);
        } catch (const std::exception& ex) {
            if (trace_enabled_) {
                std::cerr << "[LivenessAnalysisPass] Exception during stmt->accept in block " 
                          << block->id << " at statement index " << stmt_idx << ": " << ex.what() << std::endl;
            }
        } catch (...) {
            if (trace_enabled_) {
                std::cerr << "[LivenessAnalysisPass] Unknown exception during stmt->accept in block " 
                          << block->id << " at statement index " << stmt_idx << std::endl;
            }
        }
        ++stmt_idx;
    }

    use_sets_[block] = current_use_set_;
    def_sets_[block] = current_def_set_;

    if (trace_enabled_) {
        std::cout << "[LivenessAnalysisPass] Exiting analyze_block for block: " << block->id << std::endl;
    }
}