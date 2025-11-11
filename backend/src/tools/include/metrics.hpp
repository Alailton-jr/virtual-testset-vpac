#ifndef METRICS_HPP
#define METRICS_HPP

// ============================================================================
// Phase 12: Metrics and Performance Counters
// ============================================================================
// Provides thread-safe counters for observability:
// - Packet drops
// - Parse errors
// - Retransmits
// - Sent frames
// - Received frames
// - Timing outliers
//
// Usage:
//   Metrics::init();
//   Metrics::incrementPacketDrops();
//   Metrics::incrementSentFrames();
//   Metrics::recordTimingOutlier(delay_us);
//   Metrics::printSummary();
//   Metrics::reset();
// ============================================================================

#include <atomic>
#include <string>
#include <mutex>
#include <chrono>

class Metrics {
public:
    // Initialize metrics system
    static void init();
    
    // Reset all counters to zero
    static void reset();
    
    // Print metrics summary to log
    static void printSummary();
    
    // Get metrics as JSON string (for API)
    static std::string toJson();
    
    // ========== Packet Counters ==========
    
    // Increment packet drop counter
    static void incrementPacketDrops();
    
    // Increment parse error counter
    static void incrementParseErrors();
    
    // Increment retransmit counter
    static void incrementRetransmits();
    
    // Increment sent frames counter
    static void incrementSentFrames();
    
    // Increment received frames counter
    static void incrementReceivedFrames();
    
    // ========== Timing Counters ==========
    
    // Record a timing outlier (in microseconds)
    // outlier_us: Time that exceeded expected threshold
    static void recordTimingOutlier(uint64_t outlier_us);
    
    // ========== Getters ==========
    
    static uint64_t getPacketDrops();
    static uint64_t getParseErrors();
    static uint64_t getRetransmits();
    static uint64_t getSentFrames();
    static uint64_t getReceivedFrames();
    static uint64_t getTimingOutliers();
    static uint64_t getMaxTimingOutlier();  // Max outlier value in µs

private:
    Metrics() = default;
    ~Metrics() = default;
    
    static Metrics& instance();
    
    // Counters (atomic for thread safety)
    std::atomic<uint64_t> packetDrops_{0};
    std::atomic<uint64_t> parseErrors_{0};
    std::atomic<uint64_t> retransmits_{0};
    std::atomic<uint64_t> sentFrames_{0};
    std::atomic<uint64_t> receivedFrames_{0};
    std::atomic<uint64_t> timingOutliers_{0};
    std::atomic<uint64_t> maxTimingOutlier_{0};  // Microseconds
    
    std::chrono::steady_clock::time_point initTime_;
    std::mutex mutex_;
    bool initialized_{false};
};

// Convenience macros for metrics
#define METRIC_PKT_DROP()        Metrics::incrementPacketDrops()
#define METRIC_PARSE_ERROR()     Metrics::incrementParseErrors()
#define METRIC_RETRANSMIT()      Metrics::incrementRetransmits()
#define METRIC_SENT_FRAME()      Metrics::incrementSentFrames()
#define METRIC_RECV_FRAME()      Metrics::incrementReceivedFrames()
#define METRIC_TIMING_OUTLIER(us) Metrics::recordTimingOutlier(us)

#endif // METRICS_HPP
