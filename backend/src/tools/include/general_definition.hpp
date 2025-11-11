#ifndef GENERAL_DEFINITION_HPP
#define GENERAL_DEFINITION_HPP

#include <cstdlib>
#include <string>

// Phase 6: Replace #define with constexpr for type safety and debuggability
// Interface name can be overridden via IF_NAME environment variable

inline std::string getInterfaceName() {
    const char* env = std::getenv("IF_NAME");
    return env ? std::string(env) : "eth0";
}

constexpr int Sniffer_NoThreads = 1;
constexpr int Sniffer_NoTasks = 12;
constexpr int Sniffer_ThreadPriority = 80;
constexpr int Sniffer_RxSize = 2048;

constexpr int Protection_ThreadPriority = 90;

constexpr int PORT = 8080;
constexpr int MAX_CLIENTS = 10;

#endif