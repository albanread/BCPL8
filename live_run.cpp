#include "LivenessAnalysisPass.h"
#include <iostream>

void LivenessAnalysisPass::run() {
    if (trace_enabled_) {
        std::cout << "--- Running Liveness Analysis ---" << std::endl;
    }
    try {
        if (trace_enabled_) {
            std::cout << "[LivenessAnalysisPass] Calling compute_use_def_sets()" << std::endl;
        }
        compute_use_def_sets();
        if (trace_enabled_) {
            std::cout << "[LivenessAnalysisPass] Finished compute_use_def_sets()" << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[LivenessAnalysisPass] Exception in compute_use_def_sets: " << ex.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "[LivenessAnalysisPass] Unknown exception in compute_use_def_sets" << std::endl;
        throw;
    }
    try {
        if (trace_enabled_) {
            std::cout << "[LivenessAnalysisPass] Calling run_data_flow_analysis()" << std::endl;
        }
        run_data_flow_analysis();
        if (trace_enabled_) {
            std::cout << "[LivenessAnalysisPass] Finished run_data_flow_analysis()" << std::endl;
        }
    } catch (const std::exception& ex) {
        std::cerr << "[LivenessAnalysisPass] Exception in run_data_flow_analysis: " << ex.what() << std::endl;
        throw;
    } catch (...) {
        std::cerr << "[LivenessAnalysisPass] Unknown exception in run_data_flow_analysis" << std::endl;
        throw;
    }
    if (trace_enabled_) {
        std::cout << "--- Liveness Analysis Complete ---" << std::endl;
    }
}