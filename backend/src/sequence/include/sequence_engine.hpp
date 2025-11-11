#ifndef VTS_SEQUENCE_ENGINE_HPP
#define VTS_SEQUENCE_ENGINE_HPP

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <atomic>
#include <thread>
#include <functional>
#include <chrono>

namespace vts {
namespace sequence {

/**
 * @brief Transition type for sequence states
 */
enum class TransitionType {
    TIME,       // Transition after duration expires
    GOOSE_TRIP  // Transition when GOOSE trip rule triggers
};

/**
 * @brief Per-channel phasor configuration
 */
struct ChannelPhasor {
    double mag;        // Magnitude (peak or RMS depending on config)
    double angleDeg;   // Angle in degrees
    
    ChannelPhasor() : mag(0.0), angleDeg(0.0) {}
    ChannelPhasor(double m, double a) : mag(m), angleDeg(a) {}
};

/**
 * @brief Per-stream phasor state
 */
struct StreamPhasorState {
    double freq;  // Frequency in Hz
    std::map<std::string, ChannelPhasor> channels;  // Channel name -> phasor
    
    StreamPhasorState() : freq(60.0) {}
};

/**
 * @brief State transition configuration
 */
struct StateTransition {
    TransitionType type;
    
    StateTransition() : type(TransitionType::TIME) {}
    explicit StateTransition(TransitionType t) : type(t) {}
};

/**
 * @brief Single state in a sequence
 */
struct SequenceState {
    std::string name;
    double durationSec;
    StateTransition transition;
    std::map<std::string, StreamPhasorState> phasors;  // StreamID -> phasor state
    
    SequenceState() : name(""), durationSec(0.0) {}
};

/**
 * @brief Sequence definition
 */
struct Sequence {
    std::vector<std::string> activeStreams;
    std::vector<SequenceState> states;
    
    Sequence() = default;
};

/**
 * @brief Sequence execution status
 */
enum class SequenceStatus {
    IDLE,
    RUNNING,
    PAUSED,
    COMPLETED,
    STOPPED,
    ERROR
};

/**
 * @brief Progress callback function
 * 
 * Parameters: currentState, totalStates, stateName, elapsedSec, message
 */
using ProgressCallback = std::function<void(size_t, size_t, const std::string&, double, const std::string&)>;

/**
 * @brief Phasor update callback function
 * 
 * Called when sequence needs to update stream phasors
 * Parameters: streamId, phasorState
 */
using PhasorUpdateCallback = std::function<void(const std::string&, const StreamPhasorState&)>;

/**
 * @brief Sequence Engine
 * 
 * Manages execution of multi-state test sequences with time-based
 * and GOOSE-trip-based transitions. Coordinates with SV Publisher
 * Manager to apply phasor states.
 */
class SequenceEngine {
public:
    SequenceEngine();
    ~SequenceEngine();
    
    /**
     * @brief Start sequence execution
     * 
     * @param seq Sequence definition
     * @return true if started successfully, false otherwise
     */
    bool start(const Sequence& seq);
    
    /**
     * @brief Stop sequence execution
     */
    void stop();
    
    /**
     * @brief Pause sequence execution
     */
    void pause();
    
    /**
     * @brief Resume paused sequence
     */
    void resume();
    
    /**
     * @brief Get current sequence status
     * 
     * @return Current status
     */
    SequenceStatus getStatus() const;
    
    /**
     * @brief Get current state index
     * 
     * @return Current state index (0-based), or -1 if not running
     */
    int getCurrentStateIndex() const;
    
    /**
     * @brief Get elapsed time in current state
     * 
     * @return Elapsed seconds in current state
     */
    double getStateElapsedTime() const;
    
    /**
     * @brief Get total sequence elapsed time
     * 
     * @return Total elapsed seconds since sequence start
     */
    double getTotalElapsedTime() const;
    
    /**
     * @brief Set progress callback
     * 
     * @param callback Callback function for progress updates
     */
    void setProgressCallback(ProgressCallback callback);
    
    /**
     * @brief Set phasor update callback
     * 
     * @param callback Callback function for phasor updates
     */
    void setPhasorUpdateCallback(PhasorUpdateCallback callback);
    
    /**
     * @brief Get last error message
     * 
     * @return Last error message, empty if no error
     */
    std::string getLastError() const;

private:
    void sequenceThread();
    void applyState(const SequenceState& state);
    bool waitForTransition(const SequenceState& state);
    void reportProgress(const std::string& message);
    void setError(const std::string& msg);
    
    // Configuration
    Sequence sequence_;
    
    // State
    std::atomic<SequenceStatus> status_;
    std::atomic<int> currentStateIndex_;
    std::atomic<bool> stopRequested_;
    std::atomic<bool> pauseRequested_;
    
    // Timing
    std::chrono::steady_clock::time_point sequenceStartTime_;
    std::chrono::steady_clock::time_point stateStartTime_;
    
    // Thread
    std::thread sequenceThread_;
    
    // Callbacks
    ProgressCallback progressCallback_;
    PhasorUpdateCallback phasorUpdateCallback_;
    
    // Error handling
    std::string lastError_;
};

} // namespace sequence
} // namespace vts

#endif // VTS_SEQUENCE_ENGINE_HPP
