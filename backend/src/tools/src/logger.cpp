#include "logger.hpp"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

// Get singleton instance
Logger& Logger::instance() {
    static Logger logger;
    return logger;
}

// Initialize logger
void Logger::init(LogLevel level, const std::string& filename) {
    instance().initImpl(level, filename);
}

void Logger::initImpl(LogLevel level, const std::string& filename) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    minLevel_.store(level, std::memory_order_relaxed);
    
    if (!filename.empty()) {
        logFile_.open(filename, std::ios::out | std::ios::app);
        if (logFile_.is_open()) {
            fileOutput_ = true;
            std::cout << "[LOGGER] Logging to file: " << filename << std::endl;
        } else {
            std::cerr << "[LOGGER] Warning: Failed to open log file: " << filename << std::endl;
            fileOutput_ = false;
        }
    }
    
    initialized_ = true;
    
    // Don't call log() here - would cause deadlock since we're already holding mutex_
    std::cout << "[LOGGER] Logger initialized (level=" << logLevelToString(level) 
              << ", file=" << (filename.empty() ? "console" : filename) << ")" << std::endl;
}

// Shutdown logger
void Logger::shutdown() {
    instance().shutdownImpl();
}

void Logger::shutdownImpl() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (initialized_) {
        // Don't call log() here - would cause deadlock since we're already holding mutex_
        std::cout << "[LOGGER] Logger shutting down" << std::endl;
        
        if (fileOutput_ && logFile_.is_open()) {
            logFile_.flush();
            logFile_.close();
        }
        
        initialized_ = false;
    }
}

// Set log level
void Logger::setLogLevel(LogLevel level) {
    instance().setLogLevelImpl(level);
}

void Logger::setLogLevelImpl(LogLevel level) {
    minLevel_.store(level, std::memory_order_relaxed);
    // Don't call log() here - would cause deadlock since we're already holding mutex_ in some callers
    std::cout << "[LOGGER] Log level changed to " << logLevelToString(level) << std::endl;
}

// Get log level
LogLevel Logger::getLogLevel() {
    return instance().getLogLevelImpl();
}

LogLevel Logger::getLogLevelImpl() const {
    return minLevel_.load(std::memory_order_relaxed);
}

// Check if should log
bool Logger::shouldLog(LogLevel level) {
    return instance().shouldLogImpl(level);
}

bool Logger::shouldLogImpl(LogLevel level) const {
    if (!initialized_) {
        return false;
    }
    return level >= minLevel_.load(std::memory_order_relaxed);
}

// Format timestamp with microsecond precision
std::string Logger::formatTimestamp() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    
    time_t nowtime = tv.tv_sec;
    struct tm* nowtm = localtime(&nowtime);
    
    std::ostringstream oss;
    oss << std::setfill('0')
        << std::setw(4) << (nowtm->tm_year + 1900) << "-"
        << std::setw(2) << (nowtm->tm_mon + 1) << "-"
        << std::setw(2) << nowtm->tm_mday << " "
        << std::setw(2) << nowtm->tm_hour << ":"
        << std::setw(2) << nowtm->tm_min << ":"
        << std::setw(2) << nowtm->tm_sec << "."
        << std::setw(6) << tv.tv_usec;
    
    return oss.str();
}

// Format thread ID
std::string Logger::formatThreadId() {
    std::ostringstream oss;
    
#ifdef __APPLE__
    // macOS: Use pthread_threadid_np
    uint64_t tid;
    pthread_threadid_np(nullptr, &tid);
    oss << std::setfill('0') << std::setw(5) << (tid % 100000);
#else
    // Linux: Use pthread_self
    pthread_t tid = pthread_self();
    oss << std::setfill('0') << std::setw(5) << (tid % 100000);
#endif
    
    return oss.str();
}

// Log a message
void Logger::log(const char* tag, LogLevel level, const char* format, ...) {
    va_list args;
    va_start(args, format);
    instance().logImpl(tag, level, format, args);
    va_end(args);
}

void Logger::logImpl(const char* tag, LogLevel level, const char* format, va_list args) {
    if (!shouldLogImpl(level)) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Format timestamp and thread ID
    std::string timestamp = formatTimestamp();
    std::string threadId = formatThreadId();
    
    // Format the message with va_list
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), format, args);
    
    // Build log line: [TIMESTAMP] [THREAD] [LEVEL] [TAG] Message
    std::ostringstream logLine;
    logLine << "[" << timestamp << "] "
            << "[" << threadId << "] "
            << "[" << logLevelToString(level) << "] "
            << "[" << std::setw(10) << std::left << tag << "] "
            << buffer;
    
    std::string line = logLine.str();
    
    // Output to console
    if (level >= LogLevel::ERROR) {
        std::cerr << line << std::endl;
    } else {
        std::cout << line << std::endl;
    }
    
    // Output to file
    if (fileOutput_ && logFile_.is_open()) {
        logFile_ << line << std::endl;
        logFile_.flush();  // Ensure it's written immediately
    }
}
