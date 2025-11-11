#pragma once

#include "impedance_calculator.hpp"
#include <vector>
#include <string>
#include <functional>

namespace vts {
namespace testers {

/**
 * @brief Distance relay test point (R-X coordinates)
 */
struct DistancePoint {
    double R;                 // Resistance (Ω)
    double X;                 // Reactance (Ω)
    FaultType faultType;      // Type of fault
    double expectedTime;      // Expected trip time (seconds), 0 for instantaneous
    std::string label;        // Optional label for the point
};

/**
 * @brief Distance relay test result
 */
struct DistanceResult {
    bool tripped;             // Whether relay tripped
    double tripTime;          // Measured trip time (seconds)
    double R;                 // Test point R
    double X;                 // Test point X
    FaultType faultType;      // Fault type tested
    bool passed;              // Pass/fail based on expected time
    std::string error;        // Error message if failed
};

/**
 * @brief Distance relay test configuration
 */
struct DistanceTestConfig {
    std::vector<DistancePoint> points;  // Test points to evaluate
    SourceImpedance source;             // Source impedance
    double prefaultDuration;            // Pre-fault state duration (seconds)
    double faultDuration;               // Fault state duration (seconds)
    double timeTolerance;               // Trip time tolerance (seconds)
    bool stopOnFirstFailure;            // Stop test on first failure
    std::string streamId;               // SV stream ID to modify
};

/**
 * @brief Progress callback for distance test
 * @param pointIndex Current point index
 * @param totalPoints Total number of points
 * @param currentPoint Current test point
 */
using DistanceProgressCallback = std::function<void(int pointIndex, int totalPoints, 
                                                     const DistancePoint& currentPoint)>;

/**
 * @brief Distance relay tester (Zone 21)
 * 
 * Tests distance relay operation by injecting faults at specified R-X coordinates.
 * Uses impedance calculator to generate appropriate phasor states.
 */
class DistanceTester {
public:
    DistanceTester();
    ~DistanceTester();
    
    /**
     * @brief Run distance relay test
     * @param config Test configuration
     * @param progressCallback Optional progress callback
     * @return Vector of test results for each point
     */
    std::vector<DistanceResult> run(const DistanceTestConfig& config,
                                     DistanceProgressCallback progressCallback = nullptr);
    
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
     * @brief Set the phasor setter function
     * @param setter Function to set three-phase phasors
     */
    void setPhasorSetter(std::function<void(const PhasorState&)> setter);

private:
    // Internal state
    bool running_;
    bool stopRequested_;
    ImpedanceCalculator impedanceCalc_;
    
    // Callback functions
    std::function<bool()> tripFlagGetter_;
    std::function<void(const PhasorState&)> phasorSetter_;
    
    /**
     * @brief Test a single distance point
     * @param point Test point
     * @param config Test configuration
     * @return Test result for the point
     */
    DistanceResult testPoint(const DistancePoint& point,
                             const DistanceTestConfig& config);
    
    /**
     * @brief Wait for specified duration with stop check
     * @param duration Duration to wait (seconds)
     * @return true if completed, false if stopped
     */
    bool waitWithStopCheck(double duration);
    
    /**
     * @brief Monitor TRIP_FLAG for trip event
     * @param maxDuration Maximum time to wait (seconds)
     * @param tripTime Output parameter for trip time
     * @return true if tripped, false if timeout or stopped
     */
    bool monitorTrip(double maxDuration, double& tripTime);
};

} // namespace testers
} // namespace vts
