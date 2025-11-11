#include "differential_tester.hpp"
#include <thread>
#include <chrono>
#include <cmath>

namespace vts {
namespace testers {

DifferentialTester::DifferentialTester()
    : running_(false), stopRequested_(false) {
}

DifferentialTester::~DifferentialTester() {
    stop();
}

void DifferentialTester::setTripFlagGetter(std::function<bool()> getter) {
    tripFlagGetter_ = getter;
}

void DifferentialTester::setSide1CurrentSetter(std::function<void(double)> setter) {
    side1CurrentSetter_ = setter;
}

void DifferentialTester::setSide2CurrentSetter(std::function<void(double)> setter) {
    side2CurrentSetter_ = setter;
}

void DifferentialTester::stop() {
    stopRequested_ = true;
}

bool DifferentialTester::isRunning() const {
    return running_;
}

void DifferentialTester::calculateSideCurrents(double Ir, double Id, double& Is1, double& Is2) {
    // Differential relay equations:
    // Id = |Is1 + Is2|  (differential/operate current)
    // Ir = |Is1 - Is2|/2  (restraint current)
    //
    // Solving for Is1 and Is2 (assuming in-phase currents):
    // Is1 = Ir + Id/2
    // Is2 = -(Ir - Id/2)
    
    Is1 = Ir + (Id / 2.0);
    Is2 = -(Ir - (Id / 2.0));
}

bool DifferentialTester::waitWithStopCheck(double duration) {
    auto durationMs = std::chrono::milliseconds(static_cast<long long>(duration * 1000.0));
    auto start = std::chrono::steady_clock::now();
    auto end = start + durationMs;
    
    while (std::chrono::steady_clock::now() < end) {
        if (stopRequested_) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return true;
}

bool DifferentialTester::monitorTrip(double maxDuration, double& tripTime) {
    auto start = std::chrono::steady_clock::now();
    auto maxDurationMs = std::chrono::milliseconds(static_cast<long long>(maxDuration * 1000.0));
    auto end = start + maxDurationMs;
    
    bool initialTripState = tripFlagGetter_();
    
    while (std::chrono::steady_clock::now() < end) {
        if (stopRequested_) {
            return false;
        }
        
        bool currentTripState = tripFlagGetter_();
        
        // Detect trip (0 → 1 transition)
        if (!initialTripState && currentTripState) {
            auto now = std::chrono::steady_clock::now();
            tripTime = std::chrono::duration<double>(now - start).count();
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    return false;
}

DifferentialResult DifferentialTester::testPoint(const DifferentialPoint& point,
                                                  const DifferentialTestConfig& config) {
    DifferentialResult result;
    result.Ir = point.Ir;
    result.Id = point.Id;
    result.tripped = false;
    result.tripTime = 0.0;
    result.expectedTime = point.expectedTime;
    result.passed = false;
    
    try {
        // Calculate side currents
        calculateSideCurrents(point.Ir, point.Id, result.Is1, result.Is2);
        
        // Set side currents
        side1CurrentSetter_(result.Is1);
        side2CurrentSetter_(result.Is2);
        
        // Monitor for trip
        double tripTime = 0.0;
        bool tripped = monitorTrip(config.maxTestDuration, tripTime);
        
        result.tripped = tripped;
        result.tripTime = tripTime;
        
        // Check if result matches expected
        if (point.expectedTime == 0.0) {
            // Instantaneous trip expected
            result.passed = tripped && (tripTime < config.timeTolerance);
            if (!result.passed && tripped) {
                result.error = "Trip time too slow for instantaneous operation";
            }
        } else {
            // Time-delayed trip expected
            if (tripped) {
                double timeDiff = std::abs(tripTime - point.expectedTime);
                result.passed = (timeDiff <= config.timeTolerance);
                if (!result.passed) {
                    result.error = "Trip time outside tolerance";
                }
            } else {
                result.passed = false;
                result.error = "Relay did not trip within max test duration";
            }
        }
        
    } catch (const std::exception& e) {
        result.error = std::string("Exception: ") + e.what();
        result.passed = false;
    }
    
    return result;
}

std::vector<DifferentialResult> DifferentialTester::run(const DifferentialTestConfig& config,
                                                         DifferentialProgressCallback progressCallback) {
    std::vector<DifferentialResult> results;
    
    if (running_) {
        DifferentialResult errorResult;
        errorResult.error = "Test already running";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (!tripFlagGetter_) {
        DifferentialResult errorResult;
        errorResult.error = "TRIP_FLAG getter not configured";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (!side1CurrentSetter_ || !side2CurrentSetter_) {
        DifferentialResult errorResult;
        errorResult.error = "Side current setters not configured";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (config.points.empty()) {
        DifferentialResult errorResult;
        errorResult.error = "No test points provided";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    running_ = true;
    stopRequested_ = false;
    
    // Test each point
    for (size_t i = 0; i < config.points.size(); ++i) {
        if (stopRequested_) {
            DifferentialResult errorResult;
            errorResult.error = "Test stopped by user";
            errorResult.passed = false;
            results.push_back(errorResult);
            break;
        }
        
        const DifferentialPoint& point = config.points[i];
        
        // Progress callback
        if (progressCallback) {
            progressCallback(static_cast<int>(i), static_cast<int>(config.points.size()), point);
        }
        
        // Test the point
        DifferentialResult result = testPoint(point, config);
        results.push_back(result);
        
        // Stop on first failure if configured
        if (config.stopOnFirstFailure && !result.passed) {
            break;
        }
        
        // Wait a bit between tests to allow relay to reset
        if (i < config.points.size() - 1) {
            waitWithStopCheck(1.0); // 1 second between tests
        }
    }
    
    running_ = false;
    return results;
}

} // namespace testers
} // namespace vts
