#pragma once

#include <string>
#include <vector>
#include <functional>

namespace vts {
namespace testers {

/**
 * @brief Differential relay test point (Ir/Id coordinates)
 */
struct DifferentialPoint {
    double Ir;              // Restraint current (A)
    double Id;              // Differential/operate current (A)
    double expectedTime;    // Expected trip time (seconds), 0 for instantaneous
    std::string label;      // Optional label
};

/**
 * @brief Differential relay test result
 */
struct DifferentialResult {
    double Ir;              // Restraint current tested (A)
    double Id;              // Differential current tested (A)
    double Is1;             // Side 1 current (A)
    double Is2;             // Side 2 current (A)
    bool tripped;           // Whether relay tripped
    double tripTime;        // Measured trip time (seconds)
    double expectedTime;    // Expected trip time (seconds)
    bool passed;            // Pass/fail based on tolerance
    std::string error;      // Error message if failed
};

/**
 * @brief Differential relay test configuration
 */
struct DifferentialTestConfig {
    std::vector<DifferentialPoint> points;  // Test points
    double timeTolerance;                   // Trip time tolerance (seconds)
    double maxTestDuration;                 // Maximum test duration per point (seconds)
    bool stopOnFirstFailure;                // Stop test on first failure
    std::string stream1Id;                  // SV stream ID for side 1
    std::string stream2Id;                  // SV stream ID for side 2
};

/**
 * @brief Progress callback for differential test
 */
using DifferentialProgressCallback = std::function<void(int pointIndex, int totalPoints,
                                                         const DifferentialPoint& currentPoint)>;

/**
 * @brief Differential relay tester (87)
 * 
 * Tests differential relay operation using Ir/Id characteristics.
 * Converts Ir/Id to side currents: Is1 = Ir + Id/2, Is2 = -(Ir - Id/2)
 */
class DifferentialTester {
public:
    DifferentialTester();
    ~DifferentialTester();
    
    /**
     * @brief Run differential relay test
     * @param config Test configuration
     * @param progressCallback Optional progress callback
     * @return Vector of test results for each point
     */
    std::vector<DifferentialResult> run(const DifferentialTestConfig& config,
                                         DifferentialProgressCallback progressCallback = nullptr);
    
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
     * @brief Set the current setter function for side 1
     */
    void setSide1CurrentSetter(std::function<void(double)> setter);
    
    /**
     * @brief Set the current setter function for side 2
     */
    void setSide2CurrentSetter(std::function<void(double)> setter);
    
    /**
     * @brief Convert Ir/Id to side currents
     * @param Ir Restraint current
     * @param Id Differential current
     * @param Is1 Output: Side 1 current
     * @param Is2 Output: Side 2 current
     */
    static void calculateSideCurrents(double Ir, double Id, double& Is1, double& Is2);

private:
    bool running_;
    bool stopRequested_;
    
    std::function<bool()> tripFlagGetter_;
    std::function<void(double)> side1CurrentSetter_;
    std::function<void(double)> side2CurrentSetter_;
    
    /**
     * @brief Test a single differential point
     */
    DifferentialResult testPoint(const DifferentialPoint& point,
                                  const DifferentialTestConfig& config);
    
    /**
     * @brief Wait with stop check
     */
    bool waitWithStopCheck(double duration);
    
    /**
     * @brief Monitor for trip
     */
    bool monitorTrip(double maxDuration, double& tripTime);
};

} // namespace testers
} // namespace vts
