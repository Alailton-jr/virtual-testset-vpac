#include "metrics.hpp"
#include "logger.hpp"
#include <sstream>
#include <iomanip>
#include <algorithm>

// Get singleton instance
Metrics& Metrics::instance() {
    static Metrics metrics;
    return metrics;
}

// Initialize metrics
void Metrics::init() {
    auto& m = instance();
    std::lock_guard<std::mutex> lock(m.mutex_);
    
    if (!m.initialized_) {
        m.initTime_ = std::chrono::steady_clock::now();
        m.initialized_ = true;
        LOG_INFO("METRICS", "Metrics system initialized");
    }
}

// Reset all counters
void Metrics::reset() {
    auto& m = instance();
    std::lock_guard<std::mutex> lock(m.mutex_);
    
    m.packetDrops_.store(0, std::memory_order_relaxed);
    m.parseErrors_.store(0, std::memory_order_relaxed);
    m.retransmits_.store(0, std::memory_order_relaxed);
    m.sentFrames_.store(0, std::memory_order_relaxed);
    m.receivedFrames_.store(0, std::memory_order_relaxed);
    m.timingOutliers_.store(0, std::memory_order_relaxed);
    m.maxTimingOutlier_.store(0, std::memory_order_relaxed);
    
    m.initTime_ = std::chrono::steady_clock::now();
    
    LOG_INFO("METRICS", "All metrics counters reset to zero");
}

// Print metrics summary
void Metrics::printSummary() {
    auto& m = instance();
    std::lock_guard<std::mutex> lock(m.mutex_);
    
    if (!m.initialized_) {
        LOG_WARN("METRICS", "Metrics not initialized");
        return;
    }
    
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m.initTime_).count();
    double uptime_sec = uptime_ms / 1000.0;
    
    LOG_INFO("METRICS", "========== Metrics Summary ==========");
    LOG_INFO("METRICS", "Uptime: %.2f seconds", uptime_sec);
    LOG_INFO("METRICS", "Sent Frames:      %llu", (unsigned long long)m.sentFrames_.load(std::memory_order_relaxed));
    LOG_INFO("METRICS", "Received Frames:  %llu", (unsigned long long)m.receivedFrames_.load(std::memory_order_relaxed));
    LOG_INFO("METRICS", "Packet Drops:     %llu", (unsigned long long)m.packetDrops_.load(std::memory_order_relaxed));
    LOG_INFO("METRICS", "Parse Errors:     %llu", (unsigned long long)m.parseErrors_.load(std::memory_order_relaxed));
    LOG_INFO("METRICS", "Retransmits:      %llu", (unsigned long long)m.retransmits_.load(std::memory_order_relaxed));
    LOG_INFO("METRICS", "Timing Outliers:  %llu", (unsigned long long)m.timingOutliers_.load(std::memory_order_relaxed));
    
    uint64_t maxOutlier = m.maxTimingOutlier_.load(std::memory_order_relaxed);
    if (maxOutlier > 0) {
        LOG_INFO("METRICS", "Max Timing Outlier: %llu µs", (unsigned long long)maxOutlier);
    }
    
    // Calculate rates
    if (uptime_sec > 0) {
        double sendRate = m.sentFrames_.load(std::memory_order_relaxed) / uptime_sec;
        double recvRate = m.receivedFrames_.load(std::memory_order_relaxed) / uptime_sec;
        LOG_INFO("METRICS", "Send Rate:        %.2f frames/sec", sendRate);
        LOG_INFO("METRICS", "Receive Rate:     %.2f frames/sec", recvRate);
    }
    
    LOG_INFO("METRICS", "=====================================");
}

// Get metrics as JSON string
std::string Metrics::toJson() {
    auto& m = instance();
    std::lock_guard<std::mutex> lock(m.mutex_);
    
    // Calculate uptime
    auto now = std::chrono::steady_clock::now();
    auto uptime_ms = std::chrono::duration_cast<std::chrono::milliseconds>(now - m.initTime_).count();
    
    std::ostringstream json;
    json << "{\n";
    json << "  \"uptime_ms\": " << uptime_ms << ",\n";
    json << "  \"sent_frames\": " << m.sentFrames_.load(std::memory_order_relaxed) << ",\n";
    json << "  \"received_frames\": " << m.receivedFrames_.load(std::memory_order_relaxed) << ",\n";
    json << "  \"packet_drops\": " << m.packetDrops_.load(std::memory_order_relaxed) << ",\n";
    json << "  \"parse_errors\": " << m.parseErrors_.load(std::memory_order_relaxed) << ",\n";
    json << "  \"retransmits\": " << m.retransmits_.load(std::memory_order_relaxed) << ",\n";
    json << "  \"timing_outliers\": " << m.timingOutliers_.load(std::memory_order_relaxed) << ",\n";
    json << "  \"max_timing_outlier_us\": " << m.maxTimingOutlier_.load(std::memory_order_relaxed) << "\n";
    json << "}";
    
    return json.str();
}

// Increment packet drops
void Metrics::incrementPacketDrops() {
    instance().packetDrops_.fetch_add(1, std::memory_order_relaxed);
}

// Increment parse errors
void Metrics::incrementParseErrors() {
    instance().parseErrors_.fetch_add(1, std::memory_order_relaxed);
}

// Increment retransmits
void Metrics::incrementRetransmits() {
    instance().retransmits_.fetch_add(1, std::memory_order_relaxed);
}

// Increment sent frames
void Metrics::incrementSentFrames() {
    instance().sentFrames_.fetch_add(1, std::memory_order_relaxed);
}

// Increment received frames
void Metrics::incrementReceivedFrames() {
    instance().receivedFrames_.fetch_add(1, std::memory_order_relaxed);
}

// Record timing outlier
void Metrics::recordTimingOutlier(uint64_t outlier_us) {
    auto& m = instance();
    m.timingOutliers_.fetch_add(1, std::memory_order_relaxed);
    
    // Update max outlier if this is larger
    uint64_t current_max = m.maxTimingOutlier_.load(std::memory_order_relaxed);
    while (outlier_us > current_max) {
        if (m.maxTimingOutlier_.compare_exchange_weak(current_max, outlier_us, 
                                                       std::memory_order_relaxed)) {
            break;
        }
    }
}

// Getters
uint64_t Metrics::getPacketDrops() {
    return instance().packetDrops_.load(std::memory_order_relaxed);
}

uint64_t Metrics::getParseErrors() {
    return instance().parseErrors_.load(std::memory_order_relaxed);
}

uint64_t Metrics::getRetransmits() {
    return instance().retransmits_.load(std::memory_order_relaxed);
}

uint64_t Metrics::getSentFrames() {
    return instance().sentFrames_.load(std::memory_order_relaxed);
}

uint64_t Metrics::getReceivedFrames() {
    return instance().receivedFrames_.load(std::memory_order_relaxed);
}

uint64_t Metrics::getTimingOutliers() {
    return instance().timingOutliers_.load(std::memory_order_relaxed);
}

uint64_t Metrics::getMaxTimingOutlier() {
    return instance().maxTimingOutlier_.load(std::memory_order_relaxed);
}
