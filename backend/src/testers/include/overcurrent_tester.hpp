#pragma once

#include <string>
#include <vector>
#include <functional>

namespace vts {
namespace testers {

/**
 * @brief Overcurrent relay curve types (IDMT - Inverse Definite Minimum Time)
 */
enum class OCCurve {
    STANDARD_INVERSE,       // IEC Standard Inverse
    VERY_INVERSE,           // IEC Very Inverse
    EXTREMELY_INVERSE,      // IEC Extremely Inverse
    LONG_TIME_INVERSE,      // IEC Long Time Inverse
    IEEE_MODERATELY_INVERSE,// IEEE Moderately Inverse
    IEEE_VERY_INVERSE,      // IEEE Very Inverse
    IEEE_EXTREMELY_INVERSE, // IEEE Extremely Inverse
    DEFINITE_TIME,          // Definite Time (not inverse)
    INSTANTANEOUS           // Instantaneous (no time delay)
};

/**
 * @brief Overcurrent relay settings
 */
struct OCSettings {
    double pickupCurrent;   // Pickup current (A)
    double TMS;             // Time Multiplier Setting
    OCCurve curve;          // Curve type
};

/**
 * @brief Overcurrent test point
 */
struct OCPoint {
    double currentMultiple; // Current as multiple of pickup (e.g., 2.0 = 2× pickup)
    double expectedTime;    // Expected trip time (seconds)
    std::string label;      // Optional label
};

/**
 * @brief Overcurrent test result
 */
struct OCResult {
    double currentMultiple; // Tested current multiple
    double actualCurrent;   // Actual current value (A)
    bool tripped;           // Whether relay tripped
    double measuredTime;    // Measured trip time (seconds)
    double expectedTime;    // Expected trip time (seconds)
    bool passed;            // Pass/fail based on tolerance
    std::string error;      // Error message if failed
};

/**
 * @brief Overcurrent test configuration
 */
struct OCTestConfig {
    OCSettings settings;                // Relay settings
    std::vector<OCPoint> points;        // Test points
    double timeTolerance;               // Trip time tolerance (seconds or %)
    bool toleranceIsPercent;            // If true, tolerance is percentage
    double maxTestDuration;             // Maximum test duration per point (seconds)
    bool stopOnFirstFailure;            // Stop test on first failure
    std::string streamId;               // SV stream ID to modify
};

/**
 * @brief Progress callback for overcurrent test
 */
using OCProgressCallback = std::function<void(int pointIndex, int totalPoints, 
                                               const OCPoint& currentPoint)>;

/**
 * @brief Overcurrent relay tester (50/51)
 * 
 * Tests overcurrent relay operation (instantaneous 50 and time-delayed 51)
 * using standard IDMT curves.
 */
class OvercurrentTester {
public:
    OvercurrentTester();
    ~OvercurrentTester();
    
    /**
     * @brief Run overcurrent relay test
     * @param config Test configuration
     * @param progressCallback Optional progress callback
     * @return Vector of test results for each point
     */
    std::vector<OCResult> run(const OCTestConfig& config,
                               OCProgressCallback progressCallback = nullptr);
    
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
     */
    void setTripFlagGetter(std::function<bool()> getter);
    
    /**
     * @brief Set the current setter function (three-phase balanced)
     */
    void setCurrentSetter(std::function<void(double)> setter);
    
    /**
     * @brief Calculate expected trip time for a current multiple
     * @param settings Relay settings
     * @param currentMultiple Current as multiple of pickup
     * @return Expected trip time (seconds)
     */
    static double calculateTripTime(const OCSettings& settings, double currentMultiple);
    
    /**
     * @brief Parse curve type from string
     */
    static OCCurve parseCurve(const std::string& str);
    
    /**
     * @brief Convert curve type to string
     */
    static std::string curveToString(OCCurve curve);

private:
    bool running_;
    bool stopRequested_;
    
    std::function<bool()> tripFlagGetter_;
    std::function<void(double)> currentSetter_;
    
    /**
     * @brief Test a single current point
     */
    OCResult testPoint(const OCPoint& point, const OCTestConfig& config);
    
    /**
     * @brief Wait with stop check
     */
    bool waitWithStopCheck(double duration);
    
    /**
     * @brief Monitor for trip
     */
    bool monitorTrip(double maxDuration, double& tripTime);
    
    /**
     * @brief Calculate IDMT trip time using curve equation
     */
    static double calculateIDMT(OCCurve curve, double TMS, double M);
};

} // namespace testers
} // namespace vts
