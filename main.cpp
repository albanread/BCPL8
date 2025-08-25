#include "DataTypes.h"
#define _DARWIN_C_SOURCE // Required for ucontext.h on macOS

#include "RuntimeManager.h"
#include "runtime/RuntimeBridge.h"

#include "RuntimeSymbols.h"
#include <iostream>
#include "HeapManager/HeapManager.h"
#include <fstream>
#include <string>
#include <vector>
#include "HeapManager/heap_manager_defs.h"
#include <memory>
#include <stdexcept>
#include <cstring>
#include <csignal>
#include <cstdlib>
#include <sys/mman.h>
#include <unistd.h>
#include "StrengthReductionPass.h"
#include <sys/ucontext.h>
#include <sys/wait.h>
#include <execinfo.h>
#include <unistd.h>
#include "CallFrameManager.h" // Added for stack canary control

// --- Project Headers ---
#include "Preprocessor.h"
#include "AST.h"
#include "SymbolLogger.h"


#include "AssemblyWriter.h"
#include "CodeBuffer.h"
#include "LocalOptimizationPass.h"
#include "ConstantFoldingPass.h"
#include "LoopInvariantCodeMotionPass.h"
// ShortCircuitPass disabled due to memory management issues
#include "DataGenerator.h"
#include "DebugPrinter.h"
#include "InstructionStream.h"
#include "JITExecutor.h"
#include "LabelManager.h"
#include "Lexer.h"
#include "LexerDebug.h"
#include "Linker.h"
#include "NewCodeGenerator.h"
#include "Parser.h"
#include "RegisterManager.h"
#include "RuntimeManager.h"
#include "SignalSafeUtils.h"
#include "analysis/ASTAnalyzer.h"
#include "passes/ManifestResolutionPass.h"
#include "LivenessAnalysisPass.h"
#include "CFGBuilderPass.h"
#include "analysis/SymbolTableBuilder.h"
#include "runtime.h"
#include "version.h"
#include "PeepholeOptimizer.h"

// --- Formatter ---
#include "format/CodeFormatter.h"

// --- Global Variables ---
bool g_enable_lexer_trace = false;
std::unique_ptr<CodeBuffer> g_jit_code_buffer;
std::unique_ptr<JITMemoryManager> g_jit_data_manager; // <-- ADD THIS LINE
std::unique_ptr<JITExecutor> g_jit_executor;
std::string g_jit_breakpoint_label;
int g_jit_breakpoint_offset = 0;
std::string g_source_code;

// --- Forward Declarations ---
#include "include/SignalHandler.h"
std::string read_file_content(const std::string& filepath);
bool parse_arguments(int argc, char* argv[], bool& run_jit, bool& generate_asm, bool& exec_mode,
                    bool& enable_opt, bool& enable_tracing,
                    bool& trace_lexer, bool& trace_parser, bool& trace_ast, bool& trace_cfg,
                    bool& trace_codegen, bool& trace_optimizer, bool& trace_liveness,
                    bool& trace_runtime, bool& trace_symbols, bool& trace_heap,
                    bool& trace_preprocessor, bool& enable_preprocessor,
                    bool& dump_jit_stack, bool& enable_peephole, bool& enable_stack_canaries,
                    bool& format_code,
                    std::string& input_filepath, std::string& call_entry_name, int& offset_instructions,
                    std::vector<std::string>& include_paths);
void handle_static_compilation(bool exec_mode, const std::string& base_name, const InstructionStream& instruction_stream, const DataGenerator& data_generator, bool enable_debug_output);
void* handle_jit_compilation(void* jit_data_memory_base, InstructionStream& instruction_stream, int offset_instructions, bool enable_debug_output);
void handle_jit_execution(void* code_buffer_base, const std::string& call_entry_name, bool dump_jit_stack, bool enable_debug_output);

// =================================================================================
// Main Execution Logic
// =================================================================================
int main(int argc, char* argv[]) {
    ASTAnalyzer& analyzer = ASTAnalyzer::getInstance();
    bool enable_tracing = false;
    if (enable_tracing) {
        std::cout << "Debug: Starting compiler, argc=" << argc << std::endl;
    }
    bool run_jit = false;
    bool generate_asm = false;
    bool exec_mode = false;
    bool enable_opt = true;
    bool dump_jit_stack = false;
    bool enable_preprocessor = true;  // Enabled by default
    bool trace_preprocessor = false;
    bool enable_stack_canaries = false;  // Disabled by default

    // Granular tracing flags for different compiler passes
    bool trace_lexer = false;
    bool trace_parser = false;
    bool trace_ast = false;
    bool trace_cfg = false;
    bool trace_codegen = false;
    bool trace_optimizer = false;
    bool trace_liveness = false;
    bool trace_runtime = false;
    bool trace_symbols = false;
    bool trace_heap = false;
    bool enable_peephole = true; // Peephole optimizer enabled by default
    std::string input_filepath;
    std::string call_entry_name = "START";
    g_jit_breakpoint_offset = 0;
    std::vector<std::string> include_paths;
    bool format_code = false; // Add this flag

    if (enable_tracing) {
        std::cout << "Debug: About to parse arguments\n";
    }
    try {
        if (!parse_arguments(argc, argv, run_jit, generate_asm, exec_mode, enable_opt, enable_tracing,
                            trace_lexer, trace_parser, trace_ast, trace_cfg, trace_codegen,
                            trace_optimizer, trace_liveness, trace_runtime, trace_symbols, trace_heap,
                            trace_preprocessor, enable_preprocessor, dump_jit_stack, enable_peephole,
                            enable_stack_canaries,
                            // Insert format_code in the argument list
                            format_code,
                            input_filepath, call_entry_name, g_jit_breakpoint_offset, include_paths)) {
            if (enable_tracing) {
                std::cout << "Debug: parse_arguments returned false\n";
            }
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "Exception in parse_arguments: " << e.what() << std::endl;
        return 1;
    } catch (...) {
        std::cerr << "Unknown exception in parse_arguments\n";
        return 1;
    }
    if (enable_tracing) {
        std::cout << "Debug: Arguments parsed successfully\n";
    }

    // Set individual trace flags based on global tracing or specific flags
    g_enable_lexer_trace = enable_tracing || trace_lexer;
    bool enable_debug_output = enable_tracing || trace_codegen; // <-- Declare as local variable
    HeapManager::getInstance().setTraceEnabled(enable_tracing || trace_heap);
    if (enable_tracing || trace_runtime) {
        RuntimeManager::instance().enableTracing();
    }

    // Apply stack canary setting
    CallFrameManager::setStackCanariesEnabled(enable_stack_canaries);

    // Print version if any tracing is enabled
    if (enable_tracing || trace_lexer || trace_parser || trace_ast || trace_cfg ||
        trace_codegen || trace_optimizer || trace_liveness || trace_runtime ||
        trace_symbols || trace_heap) {
        print_version();
    }

    SignalHandler::setup();

    try {
        if (enable_preprocessor) {
            Preprocessor preprocessor;
            preprocessor.enableDebug(trace_preprocessor);

            // Add any include paths specified on command line
            for (const auto& path : include_paths) {
                preprocessor.addIncludePath(path);
            }

            // Add current directory as default include path
            char current_dir[PATH_MAX];
            if (getcwd(current_dir, sizeof(current_dir)) != nullptr) {
                preprocessor.addIncludePath(std::string(current_dir));
            }

            // Process the file and handle GET directives
            g_source_code = preprocessor.process(input_filepath);
        } else {
            g_source_code = read_file_content(input_filepath);
        }

        if (enable_tracing) {
            std::cout << "Compiling this source Code:\n" << g_source_code << std::endl;
        }
        // Initialize and register runtime functions using the new runtime bridge
        runtime::initialize_runtime();
        runtime::register_runtime_functions();

        if (enable_tracing) {
            std::cout << "Using " << runtime::get_runtime_version() << std::endl;
        }

        // --- Parsing and AST Construction ---
        Lexer lexer(g_source_code, enable_tracing || trace_lexer);
        Parser parser(lexer, enable_tracing || trace_parser);
        ProgramPtr ast = parser.parse_program();
        // ** NEW ERROR HANDLING BLOCK **
        if (!parser.getErrors().empty()) {
            std::cerr << "\nCompilation failed due to the following syntax error(s):" << std::endl;
            for (const auto& err : parser.getErrors()) {
                std::cerr << "  " << err << std::endl;
            }
            return 1; // Exit before code generation
        }

        if (enable_tracing || trace_parser) {
            std::cout << "Parsing complete. AST built.\n";
        }

        // --- Formatting Logic ---
        if (format_code) {
            CodeFormatter formatter;
            std::cout << formatter.format(*ast);
            return 0; // Exit after formatting
        }

        if (enable_tracing || trace_ast) {
            std::cout << "\n--- Initial Abstract Syntax Tree ---\n";
            DebugPrinter printer;
            printer.print(*ast);
            std::cout << "----------------------------------\n\n";
        }

        // --- Semantic and Optimization Passes ---
        static std::unordered_map<std::string, int64_t> g_global_manifest_constants;
        if (enable_tracing || trace_optimizer) std::cout << "Applying Manifest Resolution Pass...\n";
        ManifestResolutionPass manifest_pass(g_global_manifest_constants);
        ast = manifest_pass.apply(std::move(ast));

        // ---  Build Symbol Table ---
        if (enable_tracing || trace_symbols) std::cout << "Building symbol table...\n";
        SymbolTableBuilder symbol_table_builder(enable_tracing || trace_symbols);
        std::unique_ptr<SymbolTable> symbol_table = symbol_table_builder.build(*ast);

        // Register runtime functions in symbol table
        if (enable_tracing || trace_symbols || trace_runtime) std::cout << "Registering runtime functions in symbol table...\n";
        RuntimeSymbols::registerAll(*symbol_table);



        if (enable_opt) {
            if (enable_tracing || trace_optimizer) std::cout << "Optimization enabled. Applying passes...\n";
            ConstantFoldingPass constant_folding_pass(g_global_manifest_constants);
            ast = constant_folding_pass.apply(std::move(ast));
            StrengthReductionPass strength_reduction_pass(trace_optimizer);
            strength_reduction_pass.run(*ast);

            // (CSE pass removed; now handled after CFG construction)

            // Loop-Invariant Code Motion Pass (LICM)
            ASTAnalyzer& analyzer = ASTAnalyzer::getInstance();
            LoopInvariantCodeMotionPass licm_pass(
                g_global_manifest_constants,
                *symbol_table,
                analyzer
            );
            ast = licm_pass.apply(std::move(ast));
        }

        // TEMPORARILY DISABLED: Boolean short-circuiting pass due to memory management issues
        if (enable_tracing || trace_optimizer) std::cout << "SKIPPED: Boolean Short-Circuiting Pass...\n";

        if (enable_tracing || trace_ast) {
            std::cout << "\n--- AST After Parsing (Short-Circuiting DISABLED) ---\n";
            DebugPrinter printer;
            printer.print(*ast);
            std::cout << "----------------------------------\n\n";
        }

// Now that we have a symbol table, pass it to the analyzer
analyzer.analyze(*ast, symbol_table.get());
// --- Synchronize improved type info from analyzer to symbol table ---
for (const auto& func_pair : analyzer.get_function_metrics()) {
    const std::string& func_name = func_pair.first;
    const auto& metrics = func_pair.second;
    for (const auto& var_pair : metrics.variable_types) {
        const std::string& var_name = var_pair.first;
        VarType new_type = var_pair.second;
        symbol_table->updateSymbolType(var_name, new_type);
    }
}
if (enable_tracing || trace_ast) {
   std::cout << "Initial AST analysis complete.\n";
   analyzer.print_report();
}
if (enable_tracing || trace_symbols) {
    std::cout << "Symbol table construction complete.\n";
    symbol_table->dumpTable();
}

analyzer.transform(*ast);
if (enable_tracing || trace_ast) std::cout << "AST transformation complete.\n";



        if (enable_tracing || trace_cfg) std::cout << "Building Control Flow Graphs...\n";
        CFGBuilderPass cfg_builder(enable_tracing || trace_cfg);
        cfg_builder.build(*ast);
        if (enable_tracing || trace_cfg) {
            const auto& cfgs = cfg_builder.get_cfgs();
            for (const auto& pair : cfgs) {
                pair.second->print_cfg();
            }
        }

        // --- Local Optimization Pass (CSE/LVN) ---
        if (enable_opt) {
            if (enable_tracing || trace_optimizer) std::cout << "Applying Local Optimization Pass (CSE/LVN)...\n";
            LocalOptimizationPass local_opt_pass;
            local_opt_pass.run(cfg_builder.get_cfgs(), *symbol_table, analyzer);
        }


        if (enable_tracing || trace_liveness) std::cout << "Running Liveness Analysis...\n";
        LivenessAnalysisPass liveness_analyzer(cfg_builder.get_cfgs(), enable_tracing || trace_liveness);
        liveness_analyzer.run();
        if (enable_tracing || trace_liveness) {
            liveness_analyzer.print_results();
        }


        if (enable_tracing || trace_liveness) std::cout << "Updating register pressure from liveness data...\n";
        auto pressure_results = liveness_analyzer.calculate_register_pressure();

        // Get a mutable reference to the analyzer's metrics
        auto& function_metrics = ASTAnalyzer::getInstance().get_function_metrics_mut();

        for (const auto& pair : pressure_results) {
            const std::string& func_name = pair.first;
            int pressure = pair.second;

            if (function_metrics.count(func_name)) {
                function_metrics.at(func_name).max_live_variables = pressure;
            }
        }


        // --- Dump persistent symbol log before code generation if tracing is enabled ---
        if (enable_tracing || trace_symbols) {
            SymbolLogger::getInstance().dumpLog();
        }

        // --- Code Generation ---
        InstructionStream instruction_stream(LabelManager::instance(), enable_tracing || trace_codegen);
        DataGenerator data_generator(enable_tracing || trace_codegen);
        RegisterManager& register_manager = RegisterManager::getInstance();
        LabelManager& label_manager = LabelManager::instance();
        int debug_level = (enable_tracing || trace_codegen) ? 5 : 0;

        // --- Allocate JIT data pool before code generation ---
        const size_t JIT_DATA_POOL_SIZE = 1024 * 1024; // New 1MB size
        // supports 32k 64bit global variables.

        g_jit_data_manager = std::make_unique<JITMemoryManager>();
        g_jit_data_manager->allocate(JIT_DATA_POOL_SIZE);
        void* jit_data_memory_base = g_jit_data_manager->getMemoryPointer();
        if (!jit_data_memory_base) {
            std::cerr << "Failed to allocate JIT data pool." << std::endl;
            return 1;
        }

        // --- Code Generation ---
        NewCodeGenerator code_generator(
            instruction_stream,
            register_manager,
            label_manager,
            enable_tracing || trace_codegen,
            debug_level,
            data_generator,
            reinterpret_cast<uint64_t>(jit_data_memory_base),
            cfg_builder, // <-- Pass the CFGBuilderPass object
            std::move(symbol_table), // <-- Pass the populated symbol table
            run_jit // <-- Pass is_jit_mode: true for JIT, false for static/exec
        );
        code_generator.generate_code(*ast);
        if (enable_tracing || trace_codegen) std::cout << "Code generation complete.\n";

        // Populate the JIT data segment with initial values for globals
          if (enable_tracing || trace_codegen) std::cout << "Data sections generated.\n";


        if (enable_peephole) {
            PeepholeOptimizer peephole_optimizer(enable_tracing || trace_codegen);
            peephole_optimizer.optimize(instruction_stream);
        }


        if (generate_asm || exec_mode) {
            // FIX: Strip the file extension to get the base name.
            std::string base_name = input_filepath.substr(0, input_filepath.find_last_of('.'));
            handle_static_compilation(exec_mode, base_name, instruction_stream, data_generator, enable_tracing || trace_codegen);
        }

        // --- ADD THIS LINE TO RESET THE LABEL MANAGER ---
        LabelManager::instance().reset();
        // --- END OF ADDITION ---

        if (run_jit) {
            void* code_buffer_base = handle_jit_compilation(jit_data_memory_base, instruction_stream, g_jit_breakpoint_offset, enable_tracing || trace_codegen);

            // --- Populate the runtime function pointer table before populating the data segment and executing code ---
            RuntimeManager::instance().populate_function_pointer_table(jit_data_memory_base);

            // Make the first 512KB (runtime table) read-only
            if (g_jit_data_manager) {
                g_jit_data_manager->makeReadOnly(512 * 1024, 512 * 1024);
                if (enable_debug_output) {
                    std::cout << "Set runtime function table memory to read-only.\n";
                }
            }

            data_generator.populate_data_segment(jit_data_memory_base, label_manager);

            handle_jit_execution(code_buffer_base, call_entry_name, dump_jit_stack, enable_tracing || trace_runtime);

            // --- NEW: Call the listing functions here ---
            if (enable_tracing || trace_codegen) {
                std::cout << data_generator.generate_rodata_listing(label_manager);
                std::cout << data_generator.generate_data_listing(label_manager, jit_data_memory_base);
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "NewBCPL Compiler Error: " << ex.what() << std::endl;
        return 1;
    }

    if (enable_tracing || trace_runtime || trace_heap) {
        print_runtime_metrics();
    }

    return 0;
}

// =================================================================================
// Helper Function Implementations
// =================================================================================

/**
 * @brief Parses command-line arguments.
 * @return True if parsing is successful, false otherwise.
 */
bool parse_arguments(int argc, char* argv[], bool& run_jit, bool& generate_asm, bool& exec_mode,
                    bool& enable_opt, bool& enable_tracing,
                    bool& trace_lexer, bool& trace_parser, bool& trace_ast, bool& trace_cfg,
                    bool& trace_codegen, bool& trace_optimizer, bool& trace_liveness,
                    bool& trace_runtime, bool& trace_symbols, bool& trace_heap,
                    bool& trace_preprocessor, bool& enable_preprocessor,
                    bool& dump_jit_stack, bool& enable_peephole, bool& enable_stack_canaries,
                    bool& format_code,
                    std::string& input_filepath, std::string& call_entry_name, int& offset_instructions,
                    std::vector<std::string>& include_paths) {
    if (enable_tracing) {
        std::cout << "Debug: Entering parse_arguments with argc=" << argc << std::endl;
        std::cout << "Debug: Iterating through " << argc << " arguments\n";
    }
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (enable_tracing) {
            std::cout << "Debug: Processing argument " << i << ": " << arg << std::endl;
        }
        if (arg == "--run" || arg == "-r") run_jit = true;
        else if (arg == "--asm" || arg == "-a") generate_asm = true;
        else if (arg == "--exec" || arg == "-e") exec_mode = true;
        else if (arg == "--opt" || arg == "-o") enable_opt = true;
        else if (arg == "--nopt") enable_opt = false;
        else if (arg == "--trace" || arg == "-T") enable_tracing = true;
        else if (arg == "--trace-lexer") trace_lexer = true;
        else if (arg == "--trace-parser") trace_parser = true;
        else if (arg == "--trace-ast") trace_ast = true;
        else if (arg == "--trace-cfg") trace_cfg = true;
        else if (arg == "--trace-codegen") trace_codegen = true;
        else if (arg == "--trace-preprocessor") trace_preprocessor = true;
        else if (arg == "--trace-opt") trace_optimizer = true;
        else if (arg == "--trace-liveness") trace_liveness = true;
        else if (arg == "--popt") enable_peephole = true;
        else if (arg == "--nopeep") enable_peephole = false;
        else if (arg == "--trace-runtime") trace_runtime = true;
        else if (arg == "--trace-symbols") trace_symbols = true;
        else if (arg == "--trace-heap") trace_heap = true;
        else if (arg == "--no-preprocessor") enable_preprocessor = false;
        else if (arg == "--dump-jit-stack") dump_jit_stack = true;
        else if (arg == "--stack-canaries") enable_stack_canaries = true;
        else if (arg == "--format") format_code = true;
        else if (arg == "--nopt") enable_opt = false;
        else if (arg == "-I" || arg == "--include-path") {
            if (i + 1 < argc) include_paths.push_back(argv[++i]);
            else {
                std::cerr << "Error: -I/--include-path requires a directory path argument" << std::endl;
                return false;
            }
        }
        else if (arg == "--call" || arg == "-c") {
            if (i + 1 < argc) call_entry_name = argv[++i];
            else { std::cerr << "Error: --call option requires a name." << std::endl; return false; }
        } else if (arg == "--break" || arg == "-b") {
            if (i + 1 < argc) {
                g_jit_breakpoint_label = argv[++i];
                if (i + 1 < argc) {
                    std::string next_arg = argv[i+1];
                    if (next_arg.length() > 0 && (next_arg[0] == '+' || next_arg[0] == '-')) {
                        try { offset_instructions = std::stoi(next_arg); ++i; }
                        catch (const std::exception&) { std::cerr << "Error: Invalid offset for --break: " << next_arg << std::endl; return false; }
                    }
                }
            } else { std::cerr << "Error: --break option requires a label name." << std::endl; return false; }
        } else if (arg == "--help" || arg == "-h") {
            if (enable_tracing) {
                std::cout << "Debug: Displaying help\n";
            }
            std::cout << "Usage: " << argv[0] << " [options] <input_file.bcl>\n"
                      << "Options:\n"
                      << "  --run, -r              : JIT compile and execute the code.\n"
                      << "  --asm, -a              : Generate ARM64 assembly file.\n"
                      << "  --exec, -e             : Assemble, build with clang, and execute.\n"
                      << "  --opt, -o              : Enable AST-to-AST optimization passes (default: ON).\n"
                      << "  --nopt                 : Disable all AST-to-AST optimization passes.\n"
                      << "  --popt                 : Enable peephole optimizer (enabled by default).\n"
                      << "  --nopeep               : Disable peephole optimizer.\n"
                      << "  --no-preprocessor      : Disable GET directive processing.\n"
                      << "  --stack-canaries       : Enable stack canaries for buffer overflow detection.\n"
                      << "  -I path, --include-path path : Add directory to include search path for GET directives.\n"
                      << "                          Multiple -I flags can be specified for additional paths.\n"
                      << "                          Search order: 1) Current file's directory 2) Specified include paths\n"
                      << "  --dump-jit-stack       : Dumps the JIT stack memory after execution.\n"
                      << "  --call name, -c name   : JIT-call the routine with the given label.\n"
                      << "  --break label[+/-off]  : Insert a BRK #0 instruction at the specified label, with optional offset.\n"
                      << "  --format               : Format BCPL source code and output to stdout.\n"
                      << "  --help, -h             : Display this help message.\n"
                      << "\n"
                      << "Tracing Options (for debugging and development):\n"
                      << "  --trace, -T            : Enable all detailed tracing (verbose).\n"
                      << "  --trace-lexer          : Enable lexer tracing.\n"
                      << "  --trace-parser         : Enable parser tracing.\n"
                      << "  --trace-ast            : Enable AST building and transformation tracing.\n"
                      << "  --trace-cfg            : Enable control flow graph construction tracing.\n"
                      << "  --trace-codegen        : Enable code generation tracing.\n"
                      << "  --trace-opt            : Enable optimizer tracing.\n"
                      << "  --trace-liveness       : Enable liveness analysis tracing.\n"
                      << "  --trace-runtime        : Enable runtime function tracing.\n"
                      << "  --trace-symbols        : Enable symbol table construction tracing.\n"
                      << "  --trace-heap           : Enable heap manager tracing.\n"
                      << "  --trace-preprocessor   : Enable preprocessor tracing.\n";
            return false;
        } else if (input_filepath.empty() && arg[0] != '-') {
            input_filepath = arg;
        } else {
            std::cerr << "Error: Multiple input files specified or unknown argument: " << arg << std::endl;
            return false;
        }
    }

    if (input_filepath.empty()) {
        if (enable_tracing) {
            std::cerr << "Debug: No input file specified.\n";
        }
        std::cerr << "Error: No input file specified. Use --help for usage.\n";
        return false;
    }
    if (enable_tracing) {
        std::cout << "Debug: parse_arguments successful, input_filepath=" << input_filepath << std::endl;
    }
    return true;
}

/**
 * @brief Handles static compilation to an assembly file and optionally builds and runs it.
 */
void handle_static_compilation(bool exec_mode, const std::string& base_name, const InstructionStream& instruction_stream, const DataGenerator& data_generator, bool enable_debug_output) {
    if (enable_debug_output) std::cout << "Performing static linking for assembly file generation...\n";
    Linker static_linker;
    std::vector<Instruction> static_instructions = static_linker.process(
        instruction_stream, LabelManager::instance(), RuntimeManager::instance(), 0, nullptr, nullptr, enable_debug_output);

    std::string asm_output_path = base_name + ".s";
    AssemblyWriter asm_writer;
    asm_writer.write_to_file(asm_output_path, static_instructions, LabelManager::instance(), data_generator);

    if (exec_mode) {
        if (enable_debug_output) std::cout << "\n--- Exec Mode (via clang) ---\n";
        std::string executable_output_path = "testrun";
        std::string clang_command = "clang -g -o " + executable_output_path  + " starter.o " + asm_output_path + " bcpl_runtime.o";
       if (enable_debug_output) std::cout << "Executing: " << clang_command << std::endl;

        int build_result = system(clang_command.c_str());
        if (build_result == 0) {
            if (enable_debug_output) std::cout << "Build successful." << std::endl;
            std::string run_command = "./" + executable_output_path;
            if (enable_debug_output) std::cout << "\n--- Running '" << run_command << "' ---\n";
            int run_result = system(run_command.c_str());
            if (enable_debug_output) std::cout << "--- Program finished with exit code: " << WEXITSTATUS(run_result) << " ---\n";
        } else {
            std::cerr << "Error: Build failed with code " << build_result << std::endl;
        }
    }
}

/**
 * @brief Handles the JIT compilation process: linking, memory population, and final code commit.
 * @return A pointer to the executable code buffer.
 */
void* handle_jit_compilation(void* jit_data_memory_base, InstructionStream& instruction_stream, int offset_instructions, bool enable_debug_output) {
    if (!g_jit_code_buffer) {
        g_jit_code_buffer = std::make_unique<CodeBuffer>(32 * 1024 * 1024, enable_debug_output);
    }
    void* code_buffer_base = g_jit_code_buffer->getMemoryPointer();
    Linker jit_linker;

    // Linker runs and assigns final virtual addresses to every instruction
    std::vector<Instruction> finalized_jit_instructions = jit_linker.process(
        instruction_stream, LabelManager::instance(), RuntimeManager::instance(),
        reinterpret_cast<size_t>(code_buffer_base),
        code_buffer_base, // rodata_base is unused by linker, but pass for consistency
        jit_data_memory_base, enable_debug_output);

    if (enable_debug_output) std::cout << "Populating JIT memory according to linker layout...\n";

    // --- START OF FIX ---

    // This vector is now only for the CodeLister, not for memory population.
    std::vector<Instruction> code_and_rodata_for_listing;

    // Manually populate the JIT code and data buffers based on the linker's assigned addresses.
    for (size_t i = 0; i < finalized_jit_instructions.size(); ++i) {
        const auto& instr = finalized_jit_instructions[i];

        // Skip pseudo-instructions that are just for label definition
        if (instr.is_label_definition) {
            continue;
        }

        switch (instr.segment) {
            case SegmentType::CODE:
            case SegmentType::RODATA: {
                // Calculate the exact destination for this piece of code/data
                size_t offset = instr.address - reinterpret_cast<size_t>(code_buffer_base);
                char* dest = static_cast<char*>(code_buffer_base) + offset;

                // Copy the 32-bit instruction/data encoding to the correct location
                memcpy(dest, &instr.encoding, sizeof(uint32_t));
                code_and_rodata_for_listing.push_back(instr);
                break;
            }

            case SegmentType::DATA: {

                size_t offset = instr.address - reinterpret_cast<size_t>(jit_data_memory_base);
                char* dest = static_cast<char*>(jit_data_memory_base) + offset;

                if (instr.assembly_text.find(".quad") != std::string::npos && (i + 1) < finalized_jit_instructions.size()) {
                    const auto& upper_instr = finalized_jit_instructions[i + 1];
                    uint64_t value = (static_cast<uint64_t>(upper_instr.encoding) << 32) | instr.encoding;
                    memcpy(dest, &value, sizeof(uint64_t));
                    i++; // Skip the next instruction (upper half)
                } else {
                    memcpy(dest, &instr.encoding, sizeof(uint32_t));
                }
                break;
            }
        }
    }
    // --- END OF FIX ---

    // Set breakpoint if requested
    if (!g_jit_breakpoint_label.empty()) {
        try {
            size_t breakpoint_target_address = LabelManager::instance().get_label_address(g_jit_breakpoint_label) + (offset_instructions * 4);
            size_t offset = breakpoint_target_address - reinterpret_cast<size_t>(code_buffer_base);
            char* dest = static_cast<char*>(code_buffer_base) + offset;
            uint32_t brk_instruction = 0xD4200000; // BRK #0
            memcpy(dest, &brk_instruction, sizeof(uint32_t));
             if (enable_debug_output) std::cout << "DEBUG: Breakpoint set at 0x" << std::hex << breakpoint_target_address << std::dec << "\n";
        } catch (const std::runtime_error& e) {
            std::cerr << "Error setting breakpoint: " << e.what() << "\n";
        }
    }

    // Now, commit the memory. Pass the instruction list for debug listing purposes.
    return g_jit_code_buffer->commit(code_and_rodata_for_listing);
}


/**
 * @brief Handles the execution of the JIT-compiled code.
 */
void handle_jit_execution(void* code_buffer_base, const std::string& call_entry_name, bool dump_jit_stack, bool enable_debug_output) {
    if (!code_buffer_base) {
        std::cerr << "Cannot execute JIT code: function pointer is null." << std::endl;
        return;
    }

    if (enable_debug_output) std::cout << "\n--- JIT Execution ---\n";

    size_t entry_offset = LabelManager::instance().get_label_address(call_entry_name) - reinterpret_cast<size_t>(code_buffer_base);
    void* entry_address = static_cast<char*>(code_buffer_base) + entry_offset;

    if (enable_debug_output) std::cout << "JIT execution enabled. Entry point '" << call_entry_name << "' at " << entry_address << std::endl;

    JITFunc jit_func = reinterpret_cast<JITFunc>(entry_address);
    g_jit_executor = std::make_unique<JITExecutor>(dump_jit_stack);

    if (RuntimeManager::instance().isTracingEnabled()) {
        std::cout << "[JITExecutor] Starting execution of JIT-compiled function at address: "
                  << entry_address << std::endl;
    }

    int64_t jit_result = g_jit_executor->execute(jit_func);

    if (RuntimeManager::instance().isTracingEnabled()) {
        std::cout << "[JITExecutor] Execution completed. Result: " << jit_result << std::endl;
    }

    if (enable_debug_output) std::cout << "\n--- JIT returned with result: " << jit_result << " ---\n";
}

/**
 * @brief Reads the entire content of a file into a string.
 */
std::string read_file_content(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open file: " + filepath);
    }
    return std::string((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}



/**
 * @brief Sets up signal handlers for common fatal errors.
 */
