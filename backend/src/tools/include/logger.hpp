#ifndef LOGGER_HPP
#define LOGGER_HPP

// ============================================================================
// Phase 12: Lightweight Structured Logger
// ============================================================================
// Provides centralized logging with:
// - Log levels: DEBUG, INFO, WARN, ERROR
// - Thread-safe output
// - Timestamps (microsecond precision)
// - Thread ID tracking
// - File and console output
// - Runtime log level filtering
//
// Usage:
//   Logger::init(LogLevel::INFO, "app.log");
//   LOG_DEBUG("tag", "Debug message: %d", 42);
//   LOG_INFO("tag", "Info message");
//   LOG_WARN("tag", "Warning: %s", reason);
//   LOG_ERROR("tag", "Error: %s", error);
//   Logger::shutdown();
// ============================================================================

#include <string>
#include <fstream>
#include <mutex>
#include <chrono>
#include <cstdarg>
#include <thread>
#include <atomic>

// Log levels
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3,
    NONE = 4  // Disable all logging
};

// Convert log level to string
inline const char* logLevelToString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::NONE:  return "NONE ";
    }
    return "?????";
}

class Logger {
public:
    // Initialize logger (call once at startup)
    // level: Minimum log level to output
    // filename: Optional log file path (empty = console only)
    static void init(LogLevel level = LogLevel::INFO, const std::string& filename = "");
    
    // Shutdown logger (call at exit to flush buffers)
    static void shutdown();
    
    // Set log level at runtime
    static void setLogLevel(LogLevel level);
    
    // Get current log level
    static LogLevel getLogLevel();
    
    // Log a message with printf-style formatting
    // tag: Category/module name (e.g., "NET", "RT", "GOOSE")
    // level: Log level
    // format: Printf-style format string
    // ...: Variable arguments
    static void log(const char* tag, LogLevel level, const char* format, ...);
    
    // Check if a level would be logged (avoid expensive formatting)
    static bool shouldLog(LogLevel level);

private:
    Logger() = default;
    ~Logger() = default;
    
    static Logger& instance();
    
    void initImpl(LogLevel level, const std::string& filename);
    void shutdownImpl();
    void setLogLevelImpl(LogLevel level);
    LogLevel getLogLevelImpl() const;
    void logImpl(const char* tag, LogLevel level, const char* format, va_list args);
    bool shouldLogImpl(LogLevel level) const;
    
    std::string formatTimestamp();
    std::string formatThreadId();
    
    std::atomic<LogLevel> minLevel_{LogLevel::INFO};
    std::ofstream logFile_;
    std::mutex mutex_;
    bool initialized_{false};
    bool fileOutput_{false};
};

// Convenience macros for logging
// These automatically provide file/line context through the tag parameter

#define LOG_DEBUG(tag, ...) \
    do { \
        if (Logger::shouldLog(LogLevel::DEBUG)) { \
            Logger::log(tag, LogLevel::DEBUG, __VA_ARGS__); \
        } \
    } while(0)

#define LOG_INFO(tag, ...) \
    do { \
        if (Logger::shouldLog(LogLevel::INFO)) { \
            Logger::log(tag, LogLevel::INFO, __VA_ARGS__); \
        } \
    } while(0)

#define LOG_WARN(tag, ...) \
    do { \
        if (Logger::shouldLog(LogLevel::WARN)) { \
            Logger::log(tag, LogLevel::WARN, __VA_ARGS__); \
        } \
    } while(0)

#define LOG_ERROR(tag, ...) \
    do { \
        if (Logger::shouldLog(LogLevel::ERROR)) { \
            Logger::log(tag, LogLevel::ERROR, __VA_ARGS__); \
        } \
    } while(0)

// Legacy compatibility - gradually replace these with LOG_* macros
// These are for modules that haven't been migrated yet
#define LEGACY_LOG_INFO(msg) LOG_INFO("LEGACY", "%s", msg)
#define LEGACY_LOG_WARN(msg) LOG_WARN("LEGACY", "%s", msg)
#define LEGACY_LOG_ERROR(msg) LOG_ERROR("LEGACY", "%s", msg)

#endif // LOGGER_HPP
