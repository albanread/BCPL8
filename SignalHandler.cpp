#include "SignalHandler.h"
#include "SignalSafeUtils.h"
#include "HeapManager/HeapManager.h"
#include "JITExecutor.h"
#include <csignal>
#include <execinfo.h>
#include <cstdlib>
#include <unistd.h>
#include <cstdio>


// External globals used in handler
extern const char* g_source_code;
extern JITExecutor* g_jit_executor;

void SignalHandler::fatal_signal_handler(int signum, siginfo_t* info, void* context) {
    const char* signame;
    switch (signum) {
        case SIGSEGV: signame = "SIGSEGV"; break;
        case SIGBUS:  signame = "SIGBUS";  break;
        case SIGILL:  signame = "SIGILL";  break;
        case SIGFPE:  signame = "SIGFPE";  break;
        case SIGABRT: signame = "SIGABRT"; break;
        case SIGTRAP: signame = "SIGTRAP"; break;
        default:      signame = "UNKNOWN"; break;
    }
    safe_print("Fatal Signal (");
    safe_print(signame);
    safe_print(") caught.\n");

    if (g_source_code && *g_source_code != '\0') {
        safe_print("\n--- Source Code ---\n");
        safe_print(g_source_code);
        safe_print("\n-------------------\n\n");
    }

#if defined(__APPLE__) && defined(__aarch64__)
    ucontext_t* uc = reinterpret_cast<ucontext_t*>(context);
    __darwin_arm_thread_state64* state = &uc->uc_mcontext->__ss;

    char line_buf[128], num_buf[20], hex_buf[20];
    uint32_t cpsr = state->__cpsr;
    bool n = (cpsr >> 31) & 1, z = (cpsr >> 30) & 1, c = (cpsr >> 29) & 1, v = (cpsr >> 28) & 1;

    safe_print("\n--- Processor Flags (CPSR) ---\n");
    u64_to_hex(cpsr, hex_buf);
    snprintf(line_buf, sizeof(line_buf), "Value: %s\nFlags: N=%d Z=%d C=%d V=%d\n", hex_buf, n, z, c, v);
    safe_print(line_buf);
    safe_print("|----------------------------|\n");

    safe_print("Register dump (Apple ARM64):\n");

    for (int i = 0; i < 28; i++) {
        int_to_dec(i, num_buf);
        u64_to_hex(state->__x[i], hex_buf);
        snprintf(line_buf, sizeof(line_buf), "| X%-2s   | %-18s |\n", num_buf, hex_buf);
        safe_print(line_buf);
    }
    safe_print("|----------------------------|\n");
    u64_to_hex(state->__x[28], hex_buf); snprintf(line_buf, sizeof(line_buf), "| DP    | %-18s |\n", hex_buf); safe_print(line_buf);
    u64_to_hex(state->__fp, hex_buf); snprintf(line_buf, sizeof(line_buf), "| FP    | %-18s |\n", hex_buf); safe_print(line_buf);
    u64_to_hex(state->__lr, hex_buf); snprintf(line_buf, sizeof(line_buf), "| LR    | %-18s |\n", hex_buf); safe_print(line_buf);
    u64_to_hex(state->__sp, hex_buf); snprintf(line_buf, sizeof(line_buf), "| SP    | %-18s |\n", hex_buf); safe_print(line_buf);
    u64_to_hex(state->__pc, hex_buf); snprintf(line_buf, sizeof(line_buf), "| PC    | %-18s |\n", hex_buf); safe_print(line_buf);
    safe_print("|----------------------------|\n");

    // Attempt to access NEON/SIMD state for floating-point registers (Apple ARM64)
    safe_print("Floating-point register dump (Apple ARM64):\n");
    // On macOS, the NEON/SIMD state is not available in __darwin_arm_thread_state64.
    // Print a fallback message.
    safe_print("  [Floating-point register state not available in this context.]\n");
    safe_print("|----------------------------|\n");

    if (g_jit_executor) {
        g_jit_executor->dump_jit_stack_from_signal(state->__sp);
    }

    HeapManager::getInstance().dumpHeapSignalSafe();



#endif

    safe_print("Stack trace:\n");
    void* callstack[128];
    int frames = backtrace(callstack, 128);
    char** strs = backtrace_symbols(callstack, frames);
    if (strs) {
        for (int i = 0; i < frames; ++i) {
            safe_print("  "); safe_print(strs[i]); safe_print("\n");
        }
        free(strs);
    }
    _exit(1);
}

void SignalHandler::setup() {
    struct sigaction sa;
    sa.sa_sigaction = SignalHandler::fatal_signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO;
    sigaction(SIGSEGV, &sa, nullptr);
    sigaction(SIGBUS, &sa, nullptr);
    sigaction(SIGILL, &sa, nullptr);
    sigaction(SIGFPE, &sa, nullptr);
    sigaction(SIGABRT, &sa, nullptr);
    sigaction(SIGTRAP, &sa, nullptr);
}
