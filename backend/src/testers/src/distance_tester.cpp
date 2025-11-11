#include "distance_tester.hpp"
#include <thread>
#include <chrono>
#include <cmath>

namespace vts {
namespace testers {

DistanceTester::DistanceTester()
    : running_(false), stopRequested_(false) {
}

DistanceTester::~DistanceTester() {
    stop();
}

void DistanceTester::setTripFlagGetter(std::function<bool()> getter) {
    tripFlagGetter_ = getter;
}

void DistanceTester::setPhasorSetter(std::function<void(const PhasorState&)> setter) {
    phasorSetter_ = setter;
}

void DistanceTester::stop() {
    stopRequested_ = true;
}

bool DistanceTester::isRunning() const {
    return running_;
}

bool DistanceTester::waitWithStopCheck(double duration) {
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

bool DistanceTester::monitorTrip(double maxDuration, double& tripTime) {
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
    
    // Timeout
    return false;
}

DistanceResult DistanceTester::testPoint(const DistancePoint& point,
                                         const DistanceTestConfig& config) {
    DistanceResult result;
    result.R = point.R;
    result.X = point.X;
    result.faultType = point.faultType;
    result.tripped = false;
    result.tripTime = 0.0;
    result.passed = false;
    
    // Calculate fault impedance
    FaultImpedance faultZ;
    faultZ.R = point.R;
    faultZ.X = point.X;
    
    // Calculate pre-fault state (no fault)
    FaultImpedance noFault{0.0, 0.0};
    PhasorState prefaultState;
    
    try {
        // For pre-fault, use zero fault impedance (healthy system)
        // Just use nominal voltage, zero current
        prefaultState.voltage.A = std::complex<double>(config.source.Vprefault, 0.0);
        prefaultState.voltage.B = std::complex<double>(config.source.Vprefault, 0.0) * 
                                   std::complex<double>(-0.5, -0.866025403784439);
        prefaultState.voltage.C = std::complex<double>(config.source.Vprefault, 0.0) * 
                                   std::complex<double>(-0.5, 0.866025403784439);
        prefaultState.current.A = std::complex<double>(0.0, 0.0);
        prefaultState.current.B = std::complex<double>(0.0, 0.0);
        prefaultState.current.C = std::complex<double>(0.0, 0.0);
        
        // Apply pre-fault state
        phasorSetter_(prefaultState);
        
        // Wait for pre-fault duration
        if (!waitWithStopCheck(config.prefaultDuration)) {
            result.error = "Test stopped during pre-fault";
            return result;
        }
        
        // Calculate fault state
        PhasorState faultState = impedanceCalc_.calculateFault(point.faultType, faultZ, config.source);
        
        // Apply fault state and start timing
        phasorSetter_(faultState);
        
        // Monitor for trip
        double tripTime = 0.0;
        bool tripped = monitorTrip(config.faultDuration, tripTime);
        
        result.tripped = tripped;
        result.tripTime = tripTime;
        
        // Check if result matches expected
        if (point.expectedTime == 0.0) {
            // Instantaneous trip expected
            result.passed = tripped && (tripTime < config.timeTolerance);
        } else {
            // Time-delayed trip expected
            if (tripped) {
                double timeDiff = std::abs(tripTime - point.expectedTime);
                result.passed = (timeDiff <= config.timeTolerance);
            } else {
                result.passed = false;
                result.error = "Relay did not trip within fault duration";
            }
        }
        
    } catch (const std::exception& e) {
        result.error = std::string("Exception: ") + e.what();
        result.passed = false;
    }
    
    return result;
}

std::vector<DistanceResult> DistanceTester::run(const DistanceTestConfig& config,
                                                 DistanceProgressCallback progressCallback) {
    std::vector<DistanceResult> results;
    
    if (running_) {
        DistanceResult errorResult;
        errorResult.error = "Test already running";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (!tripFlagGetter_) {
        DistanceResult errorResult;
        errorResult.error = "TRIP_FLAG getter not configured";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (!phasorSetter_) {
        DistanceResult errorResult;
        errorResult.error = "Phasor setter not configured";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (config.points.empty()) {
        DistanceResult errorResult;
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
            DistanceResult errorResult;
            errorResult.error = "Test stopped by user";
            errorResult.passed = false;
            results.push_back(errorResult);
            break;
        }
        
        const DistancePoint& point = config.points[i];
        
        // Progress callback
        if (progressCallback) {
            progressCallback(static_cast<int>(i), static_cast<int>(config.points.size()), point);
        }
        
        // Test the point
        DistanceResult result = testPoint(point, config);
        results.push_back(result);
        
        // Stop on first failure if configured
        if (config.stopOnFirstFailure && !result.passed) {
            break;
        }
    }
    
    running_ = false;
    return results;
}

} // namespace testers
} // namespace vts
