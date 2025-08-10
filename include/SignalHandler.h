#pragma once

#include <csignal>

class SignalHandler {
public:
    // Installs fatal signal handlers
    static void setup();

    // The actual signal handler function
    static void fatal_signal_handler(int signum, siginfo_t* info, void* context);
};