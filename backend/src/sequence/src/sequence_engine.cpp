#include "sequence_engine.hpp"
#include "global_flags.hpp"
#include "logger.hpp"

#include <sstream>

namespace vts {
namespace sequence {

SequenceEngine::SequenceEngine()
    : status_(SequenceStatus::IDLE)
    , currentStateIndex_(-1)
    , stopRequested_(false)
    , pauseRequested_(false)
{
}

SequenceEngine::~SequenceEngine() {
    stop();
}

bool SequenceEngine::start(const Sequence& seq) {
    // Check if already running
    SequenceStatus currentStatus = status_.load();
    if (currentStatus == SequenceStatus::RUNNING || currentStatus == SequenceStatus::PAUSED) {
        setError("Sequence already running or paused");
        return false;
    }
    
    // Validate sequence
    if (seq.states.empty()) {
        setError("Sequence has no states");
        return false;
    }
    
    if (seq.activeStreams.empty()) {
        setError("Sequence has no active streams");
        return false;
    }
    
    // Store sequence
    sequence_ = seq;
    
    // Reset state
    currentStateIndex_.store(-1);
    stopRequested_.store(false);
    pauseRequested_.store(false);
    lastError_.clear();
    
    // Clear global trip flag
    clearTripFlag();
    
    // Start execution thread
    status_.store(SequenceStatus::RUNNING);
    sequenceThread_ = std::thread(&SequenceEngine::sequenceThread, this);
    
    LOG_INFO("SEQUENCE", "Sequence started with %zu states", seq.states.size());
    
    return true;
}

void SequenceEngine::stop() {
    SequenceStatus currentStatus = status_.load();
    if (currentStatus == SequenceStatus::IDLE || currentStatus == SequenceStatus::STOPPED) {
        return;
    }
    
    stopRequested_.store(true);
    
    if (sequenceThread_.joinable()) {
        sequenceThread_.join();
    }
    
    status_.store(SequenceStatus::STOPPED);
    currentStateIndex_.store(-1);
    
    LOG_INFO("SEQUENCE", "Sequence stopped");
}

void SequenceEngine::pause() {
    SequenceStatus currentStatus = status_.load();
    if (currentStatus != SequenceStatus::RUNNING) {
        return;
    }
    
    pauseRequested_.store(true);
    status_.store(SequenceStatus::PAUSED);
    
    LOG_INFO("SEQUENCE", "Sequence paused");
}

void SequenceEngine::resume() {
    SequenceStatus currentStatus = status_.load();
    if (currentStatus != SequenceStatus::PAUSED) {
        return;
    }
    
    pauseRequested_.store(false);
    status_.store(SequenceStatus::RUNNING);
    
    LOG_INFO("SEQUENCE", "Sequence resumed");
}

SequenceStatus SequenceEngine::getStatus() const {
    return status_.load();
}

int SequenceEngine::getCurrentStateIndex() const {
    return currentStateIndex_.load();
}

double SequenceEngine::getStateElapsedTime() const {
    if (currentStateIndex_.load() < 0) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        now - stateStartTime_
    );
    
    return elapsed.count() / 1000000.0;
}

double SequenceEngine::getTotalElapsedTime() const {
    SequenceStatus currentStatus = status_.load();
    if (currentStatus == SequenceStatus::IDLE) {
        return 0.0;
    }
    
    auto now = std::chrono::steady_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        now - sequenceStartTime_
    );
    
    return elapsed.count() / 1000000.0;
}

void SequenceEngine::setProgressCallback(ProgressCallback callback) {
    progressCallback_ = callback;
}

void SequenceEngine::setPhasorUpdateCallback(PhasorUpdateCallback callback) {
    phasorUpdateCallback_ = callback;
}

std::string SequenceEngine::getLastError() const {
    return lastError_;
}

void SequenceEngine::sequenceThread() {
    sequenceStartTime_ = std::chrono::steady_clock::now();
    
    try {
        // Execute each state in sequence
        for (size_t i = 0; i < sequence_.states.size(); i++) {
            // Check for stop request
            if (stopRequested_.load()) {
                reportProgress("Sequence stopped by user");
                status_.store(SequenceStatus::STOPPED);
                return;
            }
            
            // Wait for resume if paused
            while (pauseRequested_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (stopRequested_.load()) {
                    reportProgress("Sequence stopped while paused");
                    status_.store(SequenceStatus::STOPPED);
                    return;
                }
            }
            
            const SequenceState& state = sequence_.states[i];
            
            // Update current state index
            currentStateIndex_.store(static_cast<int>(i));
            stateStartTime_ = std::chrono::steady_clock::now();
            
            // Report state transition
            std::ostringstream oss;
            oss << "Entering state " << (i + 1) << "/" << sequence_.states.size() 
                << ": " << state.name;
            reportProgress(oss.str());
            
            LOG_INFO("SEQUENCE", "State %zu/%zu: %s (duration: %.2fs)", 
                     i + 1, sequence_.states.size(), state.name.c_str(), state.durationSec);
            
            // Apply phasor configuration for this state
            applyState(state);
            
            // Wait for transition condition
            bool transitionOccurred = waitForTransition(state);
            
            if (!transitionOccurred && stopRequested_.load()) {
                reportProgress("Sequence stopped during state execution");
                status_.store(SequenceStatus::STOPPED);
                return;
            }
        }
        
        // Sequence completed successfully
        currentStateIndex_.store(-1);
        status_.store(SequenceStatus::COMPLETED);
        reportProgress("Sequence completed successfully");
        
        LOG_INFO("SEQUENCE", "Sequence completed (total time: %.2fs)", getTotalElapsedTime());
        
    } catch (const std::exception& e) {
        std::string errorMsg = std::string("Sequence error: ") + e.what();
        setError(errorMsg);
        status_.store(SequenceStatus::ERROR);
        reportProgress(errorMsg);
        
        LOG_ERROR("SEQUENCE", "Exception: %s", e.what());
    }
}

void SequenceEngine::applyState(const SequenceState& state) {
    if (!phasorUpdateCallback_) {
        LOG_WARN("SEQUENCE", "No phasor update callback set");
        return;
    }
    
    // Apply phasor configuration for each active stream
    for (const auto& streamId : sequence_.activeStreams) {
        auto it = state.phasors.find(streamId);
        if (it != state.phasors.end()) {
            phasorUpdateCallback_(streamId, it->second);
            
            LOG_DEBUG("SEQUENCE", "Applied phasors to stream '%s' (freq: %.2f Hz, %zu channels)",
                     streamId.c_str(), it->second.freq, it->second.channels.size());
        } else {
            LOG_WARN("SEQUENCE", "State '%s' has no phasor config for stream '%s'",
                    state.name.c_str(), streamId.c_str());
        }
    }
}

bool SequenceEngine::waitForTransition(const SequenceState& state) {
    const double pollIntervalMs = 50.0;  // 50ms poll interval
    auto stateStart = std::chrono::steady_clock::now();
    auto stateDuration = std::chrono::microseconds(
        static_cast<int64_t>(state.durationSec * 1000000.0)
    );
    
    if (state.transition.type == TransitionType::TIME) {
        // Time-based transition: wait for duration to expire
        while (true) {
            // Check for stop/pause
            if (stopRequested_.load()) {
                return false;
            }
            
            // Wait for resume if paused
            while (pauseRequested_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (stopRequested_.load()) {
                    return false;
                }
            }
            
            // Check if duration expired
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - stateStart;
            
            if (elapsed >= stateDuration) {
                LOG_INFO("SEQUENCE", "Time transition: duration %.2fs expired", state.durationSec);
                return true;
            }
            
            // Sleep for poll interval
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(pollIntervalMs)
            ));
        }
        
    } else if (state.transition.type == TransitionType::GOOSE_TRIP) {
        // GOOSE trip transition: wait for global trip flag OR duration timeout
        clearTripFlag();  // Clear flag at start of state
        
        while (true) {
            // Check for stop/pause
            if (stopRequested_.load()) {
                return false;
            }
            
            // Wait for resume if paused
            while (pauseRequested_.load()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                if (stopRequested_.load()) {
                    return false;
                }
            }
            
            // Check if trip flag is set
            if (isTripFlagSet()) {
                LOG_INFO("SEQUENCE", "GOOSE trip transition: trip flag detected");
                clearTripFlag();  // Clear for next state
                return true;
            }
            
            // Check if duration expired (timeout)
            auto now = std::chrono::steady_clock::now();
            auto elapsed = now - stateStart;
            
            if (elapsed >= stateDuration) {
                LOG_WARN("SEQUENCE", "GOOSE trip transition: timeout after %.2fs (no trip detected)",
                        state.durationSec);
                return true;
            }
            
            // Sleep for poll interval
            std::this_thread::sleep_for(std::chrono::milliseconds(
                static_cast<int>(pollIntervalMs)
            ));
        }
    }
    
    return true;
}

void SequenceEngine::reportProgress(const std::string& message) {
    if (progressCallback_) {
        int stateIdx = currentStateIndex_.load();
        size_t currentState = (stateIdx >= 0) ? static_cast<size_t>(stateIdx) : 0;
        size_t totalStates = sequence_.states.size();
        std::string stateName = (stateIdx >= 0 && stateIdx < static_cast<int>(totalStates)) 
                               ? sequence_.states[currentState].name 
                               : "";
        double elapsed = getTotalElapsedTime();
        
        progressCallback_(currentState, totalStates, stateName, elapsed, message);
    }
}

void SequenceEngine::setError(const std::string& msg) {
    lastError_ = msg;
    LOG_ERROR("SEQUENCE", "%s", msg.c_str());
}

} // namespace sequence
} // namespace vts
