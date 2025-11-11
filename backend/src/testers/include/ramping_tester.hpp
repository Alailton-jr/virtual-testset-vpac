#pragma once

#include <string>
#include <functional>
#include <chrono>

namespace vts {
namespace testers {

/**
 * @brief Variable type for ramping
 */
enum class RampVariable {
    VOLTAGE_A,      // Phase A voltage magnitude
    VOLTAGE_B,      // Phase B voltage magnitude
    VOLTAGE_C,      // Phase C voltage magnitude
    VOLTAGE_3PH,    // All three-phase voltages (balanced)
    CURRENT_A,      // Phase A current magnitude
    CURRENT_B,      // Phase B current magnitude
    CURRENT_C,      // Phase C current magnitude
    CURRENT_3PH,    // All three-phase currents (balanced)
    FREQUENCY       // System frequency
};

/**
 * @brief Ramping configuration
 */
struct RampConfig {
    RampVariable variable;    // Variable to ramp
    double startValue;        // Starting value
    double endValue;          // Ending value
    double stepSize;          // Step size (increment per step)
    double stepDuration;      // Duration per step (seconds)
    bool monitorTrip;         // Whether to monitor TRIP_FLAG
    std::string streamId;     // SV stream ID to modify
};

/**
 * @brief Ramping test result
 */
struct RampResult {
    bool completed;           // Test completed successfully
    double pickupValue;       // Value at pickup (TRIP_FLAG 0→1)
    double dropoffValue;      // Value at dropoff (TRIP_FLAG 1→0)
    double resetRatio;        // Dropoff/Pickup ratio
    std::string error;        // Error message if failed
    
    // Timing information
    double pickupTime;        // Time to pickup (seconds)
    double dropoffTime;       // Time to dropoff (seconds)
    double totalDuration;     // Total test duration (seconds)
};

/**
 * @brief Progress callback for ramping test
 * @param currentValue Current ramp value
 * @param progress Progress percentage (0-100)
 * @param tripFlag Current TRIP_FLAG state
 */
using RampProgressCallback = std::function<void(double currentValue, double progress, bool tripFlag)>;

/**
 * @brief Ramping tester for pickup/dropoff/reset ratio measurement
 * 
 * Performs high-resolution ramping tests to determine relay pickup and
 * dropoff points. Monitors TRIP_FLAG for state transitions.
 */
class RampingTester {
public:
    RampingTester();
    ~RampingTester();
    
    /**
     * @brief Run a ramping test
     * @param config Ramp configuration
     * @param progressCallback Optional progress callback
     * @return Test result
     */
    RampResult run(const RampConfig& config, 
                    RampProgressCallback progressCallback = nullptr);
    
    /**
     * @brief Stop a running test
     */
    void stop();
    
    /**
     * @brief Check if a test is currently running
     */
    bool isRunning() const;
    
    /**
     * @brief Set the TRIP_FLAG getter function
     * @param getter Function that returns current TRIP_FLAG value
     */
    void setTripFlagGetter(std::function<bool()> getter);
    
    /**
     * @brief Set the value setter function
     * @param setter Function to set the ramp variable value
     */
    void setValueSetter(std::function<void(RampVariable, double)> setter);
    
    /**
     * @brief Parse ramp variable from string
     * @param str Variable name string
     * @return RampVariable enum
     */
    static RampVariable parseVariable(const std::string& str);
    
    /**
     * @brief Convert ramp variable to string
     * @param var Variable enum
     * @return String representation
     */
    static std::string variableToString(RampVariable var);

private:
    // Internal state
    bool running_;
    bool stopRequested_;
    
    // Callback functions
    std::function<bool()> tripFlagGetter_;
    std::function<void(RampVariable, double)> valueSetter_;
    
    /**
     * @brief Execute the ramping loop
     */
    RampResult executeRamp(const RampConfig& config,
                           RampProgressCallback progressCallback);
    
    /**
     * @brief Wait for specified duration with stop check
     * @param duration Duration to wait
     * @return true if completed, false if stopped
     */
    bool waitWithStopCheck(std::chrono::milliseconds duration);
};

} // namespace testers
} // namespace vts
