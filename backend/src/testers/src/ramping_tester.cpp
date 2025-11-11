#include "ramping_tester.hpp"
#include <thread>
#include <stdexcept>
#include <cmath>

namespace vts {
namespace testers {

RampingTester::RampingTester() 
    : running_(false), stopRequested_(false) {
}

RampingTester::~RampingTester() {
    stop();
}

void RampingTester::setTripFlagGetter(std::function<bool()> getter) {
    tripFlagGetter_ = getter;
}

void RampingTester::setValueSetter(std::function<void(RampVariable, double)> setter) {
    valueSetter_ = setter;
}

void RampingTester::stop() {
    stopRequested_ = true;
}

bool RampingTester::isRunning() const {
    return running_;
}

RampVariable RampingTester::parseVariable(const std::string& str) {
    if (str == "VOLTAGE_A" || str == "Va" || str == "va") return RampVariable::VOLTAGE_A;
    if (str == "VOLTAGE_B" || str == "Vb" || str == "vb") return RampVariable::VOLTAGE_B;
    if (str == "VOLTAGE_C" || str == "Vc" || str == "vc") return RampVariable::VOLTAGE_C;
    if (str == "VOLTAGE_3PH" || str == "V3ph" || str == "v3ph") return RampVariable::VOLTAGE_3PH;
    if (str == "CURRENT_A" || str == "Ia" || str == "ia") return RampVariable::CURRENT_A;
    if (str == "CURRENT_B" || str == "Ib" || str == "ib") return RampVariable::CURRENT_B;
    if (str == "CURRENT_C" || str == "Ic" || str == "ic") return RampVariable::CURRENT_C;
    if (str == "CURRENT_3PH" || str == "I3ph" || str == "i3ph") return RampVariable::CURRENT_3PH;
    if (str == "FREQUENCY" || str == "freq" || str == "f") return RampVariable::FREQUENCY;
    
    throw std::invalid_argument("Unknown ramp variable: " + str);
}

std::string RampingTester::variableToString(RampVariable var) {
    switch (var) {
        case RampVariable::VOLTAGE_A: return "VOLTAGE_A";
        case RampVariable::VOLTAGE_B: return "VOLTAGE_B";
        case RampVariable::VOLTAGE_C: return "VOLTAGE_C";
        case RampVariable::VOLTAGE_3PH: return "VOLTAGE_3PH";
        case RampVariable::CURRENT_A: return "CURRENT_A";
        case RampVariable::CURRENT_B: return "CURRENT_B";
        case RampVariable::CURRENT_C: return "CURRENT_C";
        case RampVariable::CURRENT_3PH: return "CURRENT_3PH";
        case RampVariable::FREQUENCY: return "FREQUENCY";
        default: return "UNKNOWN";
    }
}

bool RampingTester::waitWithStopCheck(std::chrono::milliseconds duration) {
    auto start = std::chrono::steady_clock::now();
    auto end = start + duration;
    
    while (std::chrono::steady_clock::now() < end) {
        if (stopRequested_) {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    return true;
}

RampResult RampingTester::executeRamp(const RampConfig& config,
                                      RampProgressCallback progressCallback) {
    RampResult result;
    result.completed = false;
    result.pickupValue = 0.0;
    result.dropoffValue = 0.0;
    result.resetRatio = 0.0;
    result.pickupTime = 0.0;
    result.dropoffTime = 0.0;
    result.totalDuration = 0.0;
    
    // Validate configuration
    if (!valueSetter_) {
        result.error = "Value setter not configured";
        return result;
    }
    
    if (config.monitorTrip && !tripFlagGetter_) {
        result.error = "TRIP_FLAG getter not configured but monitoring requested";
        return result;
    }
    
    if (std::abs(config.stepSize) < 1e-9) {
        result.error = "Step size too small";
        return result;
    }
    
    // Determine ramp direction
    bool increasing = config.endValue > config.startValue;
    if ((increasing && config.stepSize < 0) || (!increasing && config.stepSize > 0)) {
        result.error = "Step size direction doesn't match start/end values";
        return result;
    }
    
    // Calculate number of steps
    double range = std::abs(config.endValue - config.startValue);
    int numSteps = static_cast<int>(std::ceil(range / std::abs(config.stepSize)));
    
    if (numSteps < 1) {
        result.error = "Invalid number of steps";
        return result;
    }
    
    // Start timing
    auto testStart = std::chrono::steady_clock::now();
    
    // State tracking
    bool prevTripFlag = false;
    bool pickupDetected = false;
    bool dropoffDetected = false;
    
    if (config.monitorTrip) {
        prevTripFlag = tripFlagGetter_();
    }
    
    // Ramping loop
    double currentValue = config.startValue;
    
    for (int step = 0; step <= numSteps; ++step) {
        // Check for stop request
        if (stopRequested_) {
            result.error = "Test stopped by user";
            return result;
        }
        
        // Update value
        valueSetter_(config.variable, currentValue);
        
        // Wait for step duration
        auto stepDuration = std::chrono::milliseconds(
            static_cast<long long>(config.stepDuration * 1000.0));
        
        if (!waitWithStopCheck(stepDuration)) {
            result.error = "Test stopped by user";
            return result;
        }
        
        // Check TRIP_FLAG if monitoring
        bool currentTripFlag = false;
        if (config.monitorTrip) {
            currentTripFlag = tripFlagGetter_();
            
            // Detect pickup (0 → 1 transition)
            if (!prevTripFlag && currentTripFlag && !pickupDetected) {
                pickupDetected = true;
                result.pickupValue = currentValue;
                auto now = std::chrono::steady_clock::now();
                result.pickupTime = std::chrono::duration<double>(now - testStart).count();
            }
            
            // Detect dropoff (1 → 0 transition)
            if (prevTripFlag && !currentTripFlag && !dropoffDetected) {
                dropoffDetected = true;
                result.dropoffValue = currentValue;
                auto now = std::chrono::steady_clock::now();
                result.dropoffTime = std::chrono::duration<double>(now - testStart).count();
            }
            
            prevTripFlag = currentTripFlag;
        }
        
        // Progress callback
        if (progressCallback) {
            double progress = (step * 100.0) / numSteps;
            progressCallback(currentValue, progress, currentTripFlag);
        }
        
        // Increment value
        if (step < numSteps) {
            currentValue += config.stepSize;
            
            // Clamp to end value
            if (increasing && currentValue > config.endValue) {
                currentValue = config.endValue;
            } else if (!increasing && currentValue < config.endValue) {
                currentValue = config.endValue;
            }
        }
    }
    
    // Calculate total duration
    auto testEnd = std::chrono::steady_clock::now();
    result.totalDuration = std::chrono::duration<double>(testEnd - testStart).count();
    
    // Calculate reset ratio if both pickup and dropoff detected
    if (pickupDetected && dropoffDetected && std::abs(result.pickupValue) > 1e-9) {
        result.resetRatio = result.dropoffValue / result.pickupValue;
    }
    
    result.completed = true;
    return result;
}

RampResult RampingTester::run(const RampConfig& config,
                              RampProgressCallback progressCallback) {
    if (running_) {
        RampResult result;
        result.completed = false;
        result.error = "Test already running";
        return result;
    }
    
    running_ = true;
    stopRequested_ = false;
    
    RampResult result = executeRamp(config, progressCallback);
    
    running_ = false;
    return result;
}

} // namespace testers
} // namespace vts
