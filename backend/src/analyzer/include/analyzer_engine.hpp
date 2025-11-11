#ifndef VTS_ANALYZER_ENGINE_HPP
#define VTS_ANALYZER_ENGINE_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <functional>
#include <chrono>
#include <deque>

namespace vts {
namespace analyzer {

/**
 * @brief Phasor measurement result
 */
struct PhasorMeasurement {
    double magnitude;     // RMS magnitude
    double angleDeg;      // Angle in degrees
    double frequency;     // Measured frequency in Hz
    
    PhasorMeasurement() : magnitude(0.0), angleDeg(0.0), frequency(60.0) {}
    PhasorMeasurement(double mag, double angle, double freq) 
        : magnitude(mag), angleDeg(angle), frequency(freq) {}
};

/**
 * @brief Harmonic component
 */
struct HarmonicComponent {
    int order;           // Harmonic order (1=fundamental, 2=2nd, etc.)
    double magnitude;    // RMS magnitude
    double angleDeg;     // Angle in degrees
    double thd;          // Total Harmonic Distortion (%)
    
    HarmonicComponent() : order(1), magnitude(0.0), angleDeg(0.0), thd(0.0) {}
};

/**
 * @brief Channel analysis result
 */
struct ChannelAnalysis {
    std::string channelName;
    PhasorMeasurement fundamental;
    std::vector<HarmonicComponent> harmonics;  // Orders 2-15
    double rms;                                 // Total RMS
    double thd;                                 // Total Harmonic Distortion %
    
    ChannelAnalysis() : channelName(""), rms(0.0), thd(0.0) {}
};

/**
 * @brief Complete analysis frame
 */
struct AnalysisFrame {
    std::chrono::steady_clock::time_point timestamp;
    std::string streamId;
    std::vector<ChannelAnalysis> channels;
    int sampleRate;
    int samplesPerCycle;
    
    AnalysisFrame() : streamId(""), sampleRate(4800), samplesPerCycle(80) {}
};

/**
 * @brief Waveform data for oscilloscope display
 */
struct WaveformData {
    std::string channelName;
    std::vector<double> samples;      // Raw sample values
    std::vector<double> timestamps;   // Timestamps in seconds
    int sampleRate;
    
    WaveformData() : channelName(""), sampleRate(4800) {}
};

/**
 * @brief Analysis callback function
 * 
 * Called when new analysis results are available
 */
using AnalysisCallback = std::function<void(const AnalysisFrame&)>;

/**
 * @brief Waveform callback function
 * 
 * Called when new waveform data is available
 */
using WaveformCallback = std::function<void(const std::vector<WaveformData>&)>;

/**
 * @brief Ring buffer for sample storage
 */
template<typename T>
class RingBuffer {
public:
    explicit RingBuffer(size_t capacity) 
        : buffer_(capacity), head_(0), size_(0), capacity_(capacity) {}
    
    void push(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);
        buffer_[head_] = value;
        head_ = (head_ + 1) % capacity_;
        if (size_ < capacity_) {
            size_++;
        }
    }
    
    std::vector<T> getAll() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<T> result;
        result.reserve(size_);
        
        size_t start = (head_ + capacity_ - size_) % capacity_;
        for (size_t i = 0; i < size_; i++) {
            result.push_back(buffer_[(start + i) % capacity_]);
        }
        return result;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_;
    }
    
    bool isFull() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return size_ == capacity_;
    }
    
    void clear() {
        std::lock_guard<std::mutex> lock(mutex_);
        head_ = 0;
        size_ = 0;
    }

private:
    std::vector<T> buffer_;
    size_t head_;
    size_t size_;
    size_t capacity_;
    mutable std::mutex mutex_;
};

/**
 * @brief Analyzer Engine
 * 
 * Sniffs SV packets, buffers samples, performs FFT-based phasor computation,
 * and streams results via callbacks.
 */
class AnalyzerEngine {
public:
    AnalyzerEngine();
    ~AnalyzerEngine();
    
    /**
     * @brief Start analyzing a specific SV stream
     * 
     * @param streamMac MAC address of SV stream to analyze (e.g., "01:0C:CD:04:00:02")
     * @param sampleRate Expected sample rate in Hz (default 4800)
     * @return true if started successfully, false otherwise
     */
    bool start(const std::string& streamMac, int sampleRate = 4800);
    
    /**
     * @brief Stop analysis
     */
    void stop();
    
    /**
     * @brief Check if analyzer is running
     * 
     * @return true if analyzing, false otherwise
     */
    bool isRunning() const;
    
    /**
     * @brief Get currently analyzed stream MAC
     * 
     * @return Stream MAC address or empty string if not running
     */
    std::string getStreamMac() const;
    
    /**
     * @brief Set analysis callback
     * 
     * @param callback Function called with analysis results
     */
    void setAnalysisCallback(AnalysisCallback callback);
    
    /**
     * @brief Set waveform callback
     * 
     * @param callback Function called with waveform data
     */
    void setWaveformCallback(WaveformCallback callback);
    
    /**
     * @brief Process incoming SV sample
     * 
     * Called by sniffer when SV packet is received
     * 
     * @param streamMac MAC address of stream
     * @param channelName Channel identifier
     * @param value Sample value
     * @param timestamp Sample timestamp
     */
    void processSample(const std::string& streamMac, const std::string& channelName,
                      double value, std::chrono::steady_clock::time_point timestamp);
    
    /**
     * @brief Get last error message
     * 
     * @return Last error message, empty if no error
     */
    std::string getLastError() const;

private:
    void analysisThread();
    void performFFT(const std::vector<double>& samples, std::vector<double>& magnitudes, 
                   std::vector<double>& phases);
    ChannelAnalysis analyzeChannel(const std::string& channelName, 
                                   const std::vector<double>& samples);
    double computeFrequency(const std::vector<double>& samples, int sampleRate);
    void sendWaveformData();
    void setError(const std::string& msg);
    
    // Configuration
    std::string streamMac_;
    int sampleRate_;
    int samplesPerCycle_;
    
    // State
    std::atomic<bool> running_;
    std::atomic<bool> stopRequested_;
    
    // Sample buffers (per channel)
    std::map<std::string, std::shared_ptr<RingBuffer<std::pair<double, std::chrono::steady_clock::time_point>>>> channelBuffers_;
    std::mutex buffersMutex_;
    
    // Analysis thread
    std::thread analysisThread_;
    
    // Callbacks
    AnalysisCallback analysisCallback_;
    WaveformCallback waveformCallback_;
    std::mutex callbackMutex_;
    
    // Error handling
    std::string lastError_;
    mutable std::mutex errorMutex_;
    
    // Timing
    std::chrono::steady_clock::time_point lastAnalysisTime_;
};

} // namespace analyzer
} // namespace vts

#endif // VTS_ANALYZER_ENGINE_HPP
