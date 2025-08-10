#include "LivenessAnalysisPass.h"
#include <iostream>

void LivenessAnalysisPass::print_results() const {
    for (const auto& cfg_pair : cfgs_) {
        std::cout << "\nLiveness Analysis for function: " << cfg_pair.first << "\n";
        std::cout << "-------------------------------------------\n";
        for (const auto& block_pair : cfg_pair.second->blocks) {
            BasicBlock* b = block_pair.second.get();
            std::cout << "Block ID: " << b->id << "\n";
            
            // Print Use set
            std::cout << "  Use: { ";
            for(const auto& var : use_sets_.at(b)) std::cout << var << " ";
            std::cout << "}\n";

            // Print Def set
            std::cout << "  Def: { ";
            for(const auto& var : def_sets_.at(b)) std::cout << var << " ";
            std::cout << "}\n";

            // Print In set
            std::cout << "  In : { ";
            for(const auto& var : get_in_set(b)) std::cout << var << " ";
            std::cout << "}\n";

            // Print Out set
            std::cout << "  Out: { ";
            for(const auto& var : get_out_set(b)) std::cout << var << " ";
            std::cout << "}\n";
        }
    }
}