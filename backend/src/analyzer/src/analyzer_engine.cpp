#include "analyzer_engine.hpp"
#include "logger.hpp"
#include <cmath>
#include <algorithm>
#include <numeric>
#include <string>

namespace vts {
namespace analyzer {

// Constants
static constexpr double PI = 3.14159265358979323846;
static constexpr int MAX_HARMONICS = 15;
static constexpr int WAVEFORM_UPDATE_RATE_MS = 16; // ~60 Hz
static constexpr int ANALYSIS_UPDATE_RATE_MS = 100; // 10 Hz

AnalyzerEngine::AnalyzerEngine()
    : streamMac_(""),
      sampleRate_(4800),
      samplesPerCycle_(80),
      running_(false),
      stopRequested_(false),
      lastError_("") {
}

AnalyzerEngine::~AnalyzerEngine() {
    stop();
}

bool AnalyzerEngine::start(const std::string& streamMac, int sampleRate) {
    if (running_.load()) {
        setError("Analyzer already running");
        return false;
    }
    
    if (streamMac.empty()) {
        setError("Stream MAC address cannot be empty");
        return false;
    }
    
    if (sampleRate <= 0) {
        setError("Invalid sample rate");
        return false;
    }
    
    streamMac_ = streamMac;
    sampleRate_ = sampleRate;
    samplesPerCycle_ = sampleRate / 60; // Assuming 60 Hz
    
    // Clear existing buffers
    {
        std::lock_guard<std::mutex> lock(buffersMutex_);
        channelBuffers_.clear();
    }
    
    running_.store(true);
    stopRequested_.store(false);
    lastAnalysisTime_ = std::chrono::steady_clock::now();
    
    // Start analysis thread
    analysisThread_ = std::thread(&AnalyzerEngine::analysisThread, this);
    
    LOG_INFO("ANALYZER", "Started analyzing stream: {} at {} Hz", streamMac_.c_str(), sampleRate_);
    
    return true;
}

void AnalyzerEngine::stop() {
    if (!running_.load()) {
        return;
    }
    
    stopRequested_.store(true);
    running_.store(false);
    
    if (analysisThread_.joinable()) {
        analysisThread_.join();
    }
    
    LOG_INFO("ANALYZER", "Stopped analyzing stream: {}", streamMac_.c_str());
    
    streamMac_.clear();
}

bool AnalyzerEngine::isRunning() const {
    return running_.load();
}

std::string AnalyzerEngine::getStreamMac() const {
    return streamMac_;
}

void AnalyzerEngine::setAnalysisCallback(AnalysisCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    analysisCallback_ = callback;
}

void AnalyzerEngine::setWaveformCallback(WaveformCallback callback) {
    std::lock_guard<std::mutex> lock(callbackMutex_);
    waveformCallback_ = callback;
}

void AnalyzerEngine::processSample(const std::string& streamMac, const std::string& channelName,
                                  double value, std::chrono::steady_clock::time_point timestamp) {
    if (!running_.load()) {
        return;
    }
    
    // Only process samples from the target stream
    if (streamMac != streamMac_) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(buffersMutex_);
    
    // Get or create buffer for this channel
    auto it = channelBuffers_.find(channelName);
    if (it == channelBuffers_.end()) {
        // Create buffer with capacity for 2 cycles
        size_t capacity = samplesPerCycle_ * 2;
        auto buffer = std::make_shared<RingBuffer<std::pair<double, std::chrono::steady_clock::time_point>>>(capacity);
        channelBuffers_[channelName] = buffer;
        it = channelBuffers_.find(channelName);
        
        LOG_INFO("ANALYZER", "Created buffer for channel: {} (capacity: {})", channelName.c_str(), capacity);
    }
    
    // Add sample to buffer
    it->second->push(std::make_pair(value, timestamp));
}

std::string AnalyzerEngine::getLastError() const {
    std::lock_guard<std::mutex> lock(errorMutex_);
    return lastError_;
}

void AnalyzerEngine::setError(const std::string& msg) {
    std::lock_guard<std::mutex> lock(errorMutex_);
    lastError_ = msg;
    LOG_ERROR("ANALYZER", "{}", msg.c_str());
}

void AnalyzerEngine::analysisThread() {
    LOG_INFO("ANALYZER", "Analysis thread started");
    
    auto lastWaveformUpdate = std::chrono::steady_clock::now();
    auto lastAnalysisUpdate = std::chrono::steady_clock::now();
    
    while (!stopRequested_.load()) {
        auto now = std::chrono::steady_clock::now();
        
        // Send waveform data at ~60 Hz
        auto waveformElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastWaveformUpdate).count();
        
        if (waveformElapsed >= WAVEFORM_UPDATE_RATE_MS) {
            sendWaveformData();
            lastWaveformUpdate = now;
        }
        
        // Perform analysis at 10 Hz
        auto analysisElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - lastAnalysisUpdate).count();
        
        if (analysisElapsed >= ANALYSIS_UPDATE_RATE_MS) {
            // Perform FFT-based analysis
            AnalysisFrame frame;
            frame.timestamp = now;
            frame.streamId = streamMac_;
            frame.sampleRate = sampleRate_;
            frame.samplesPerCycle = samplesPerCycle_;
            
            {
                std::lock_guard<std::mutex> lock(buffersMutex_);
                
                for (const auto& [channelName, buffer] : channelBuffers_) {
                    // Need at least one cycle of data
                    if (buffer->size() >= static_cast<size_t>(samplesPerCycle_)) {
                        auto samples = buffer->getAll();
                        
                        // Extract just the values
                        std::vector<double> values;
                        values.reserve(samples.size());
                        for (const auto& sample : samples) {
                            values.push_back(sample.first);
                        }
                        
                        // Analyze this channel
                        ChannelAnalysis analysis = analyzeChannel(channelName, values);
                        frame.channels.push_back(analysis);
                    }
                }
            }
            
            // Send analysis results if we have data
            if (!frame.channels.empty()) {
                std::lock_guard<std::mutex> lock(callbackMutex_);
                if (analysisCallback_) {
                    analysisCallback_(frame);
                }
            }
            
            lastAnalysisUpdate = now;
        }
        
        // Sleep briefly to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    
    LOG_INFO("ANALYZER", "Analysis thread stopped");
}

void AnalyzerEngine::performFFT(const std::vector<double>& samples, 
                               std::vector<double>& magnitudes, 
                               std::vector<double>& phases) {
    size_t N = samples.size();
    magnitudes.resize(N / 2 + 1);
    phases.resize(N / 2 + 1);
    
    // Simple DFT implementation (can be replaced with kissfft for performance)
    for (size_t k = 0; k <= N / 2; k++) {
        double real = 0.0;
        double imag = 0.0;
        
        for (size_t n = 0; n < N; n++) {
            double angle = 2.0 * PI * k * n / N;
            real += samples[n] * std::cos(angle);
            imag -= samples[n] * std::sin(angle);
        }
        
        // Normalize
        real /= N;
        imag /= N;
        
        // Convert to magnitude and phase
        magnitudes[k] = 2.0 * std::sqrt(real * real + imag * imag); // Factor of 2 for single-sided
        phases[k] = std::atan2(imag, real) * 180.0 / PI; // Convert to degrees
        
        // DC component doesn't get doubled
        if (k == 0) {
            magnitudes[k] /= 2.0;
        }
    }
}

ChannelAnalysis AnalyzerEngine::analyzeChannel(const std::string& channelName,
                                              const std::vector<double>& samples) {
    ChannelAnalysis result;
    result.channelName = channelName;
    
    if (samples.empty()) {
        return result;
    }
    
    // Use exactly one cycle for analysis
    size_t analysisSize = std::min(samples.size(), static_cast<size_t>(samplesPerCycle_));
    std::vector<double> cycleData(samples.end() - analysisSize, samples.end());
    
    // Perform FFT
    std::vector<double> magnitudes, phases;
    performFFT(cycleData, magnitudes, phases);
    
    // Extract fundamental (assuming 60 Hz)
    // Fundamental is at bin 1 for 1-cycle FFT
    if (magnitudes.size() > 1) {
        double fundamentalMag = magnitudes[1];
        double fundamentalPhase = phases[1];
        
        // Convert peak to RMS
        double rms = fundamentalMag / std::sqrt(2.0);
        
        result.fundamental = PhasorMeasurement(rms, fundamentalPhase, 60.0);
        
        // Compute frequency using zero-crossing method or phase derivative
        result.fundamental.frequency = computeFrequency(cycleData, sampleRate_);
        
        // Extract harmonics (2nd through 15th)
        double sumHarmonicSquared = 0.0;
        for (int h = 2; h <= MAX_HARMONICS && h < static_cast<int>(magnitudes.size()); h++) {
            HarmonicComponent harmonic;
            harmonic.order = h;
            harmonic.magnitude = magnitudes[h] / std::sqrt(2.0); // RMS
            harmonic.angleDeg = phases[h];
            
            sumHarmonicSquared += harmonic.magnitude * harmonic.magnitude;
            
            result.harmonics.push_back(harmonic);
        }
        
        // Compute total RMS (DC + fundamental + harmonics)
        double totalRmsSquared = 0.0;
        for (const auto& mag : magnitudes) {
            double rmsComponent = mag / std::sqrt(2.0);
            totalRmsSquared += rmsComponent * rmsComponent;
        }
        result.rms = std::sqrt(totalRmsSquared);
        
        // Compute THD
        if (rms > 0.0) {
            result.thd = 100.0 * std::sqrt(sumHarmonicSquared) / rms;
        } else {
            result.thd = 0.0;
        }
    }
    
    return result;
}

double AnalyzerEngine::computeFrequency(const std::vector<double>& samples, int sampleRate) {
    if (samples.size() < 3) {
        return 60.0; // Default
    }
    
    // Count zero crossings
    int crossings = 0;
    for (size_t i = 1; i < samples.size(); i++) {
        if ((samples[i-1] < 0 && samples[i] >= 0) || 
            (samples[i-1] >= 0 && samples[i] < 0)) {
            crossings++;
        }
    }
    
    if (crossings < 2) {
        return 60.0; // Not enough crossings
    }
    
    // Two crossings per cycle
    double cycles = crossings / 2.0;
    double duration = static_cast<double>(samples.size()) / sampleRate;
    
    return cycles / duration;
}

void AnalyzerEngine::sendWaveformData() {
    std::vector<WaveformData> waveforms;
    
    {
        std::lock_guard<std::mutex> lock(buffersMutex_);
        
        for (const auto& [channelName, buffer] : channelBuffers_) {
            if (buffer->size() > 0) {
                auto samples = buffer->getAll();
                
                WaveformData wf;
                wf.channelName = channelName;
                wf.sampleRate = sampleRate_;
                
                // Extract values and compute timestamps
                wf.samples.reserve(samples.size());
                wf.timestamps.reserve(samples.size());
                
                auto firstTimestamp = samples[0].second;
                for (const auto& sample : samples) {
                    wf.samples.push_back(sample.first);
                    
                    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                        sample.second - firstTimestamp);
                    wf.timestamps.push_back(elapsed.count() / 1000000.0);
                }
                
                waveforms.push_back(wf);
            }
        }
    }
    
    if (!waveforms.empty()) {
        std::lock_guard<std::mutex> lock(callbackMutex_);
        if (waveformCallback_) {
            waveformCallback_(waveforms);
        }
    }
}

} // namespace analyzer
} // namespace vts
