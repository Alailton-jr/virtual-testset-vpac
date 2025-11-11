#include "overcurrent_tester.hpp"
#include <thread>
#include <chrono>
#include <cmath>
#include <stdexcept>

namespace vts {
namespace testers {

OvercurrentTester::OvercurrentTester()
    : running_(false), stopRequested_(false) {
}

OvercurrentTester::~OvercurrentTester() {
    stop();
}

void OvercurrentTester::setTripFlagGetter(std::function<bool()> getter) {
    tripFlagGetter_ = getter;
}

void OvercurrentTester::setCurrentSetter(std::function<void(double)> setter) {
    currentSetter_ = setter;
}

void OvercurrentTester::stop() {
    stopRequested_ = true;
}

bool OvercurrentTester::isRunning() const {
    return running_;
}

OCCurve OvercurrentTester::parseCurve(const std::string& str) {
    if (str == "SI" || str == "STANDARD_INVERSE") return OCCurve::STANDARD_INVERSE;
    if (str == "VI" || str == "VERY_INVERSE") return OCCurve::VERY_INVERSE;
    if (str == "EI" || str == "EXTREMELY_INVERSE") return OCCurve::EXTREMELY_INVERSE;
    if (str == "LTI" || str == "LONG_TIME_INVERSE") return OCCurve::LONG_TIME_INVERSE;
    if (str == "MI" || str == "IEEE_MODERATELY_INVERSE") return OCCurve::IEEE_MODERATELY_INVERSE;
    if (str == "IEEE_VI" || str == "IEEE_VERY_INVERSE") return OCCurve::IEEE_VERY_INVERSE;
    if (str == "IEEE_EI" || str == "IEEE_EXTREMELY_INVERSE") return OCCurve::IEEE_EXTREMELY_INVERSE;
    if (str == "DT" || str == "DEFINITE_TIME") return OCCurve::DEFINITE_TIME;
    if (str == "INST" || str == "INSTANTANEOUS") return OCCurve::INSTANTANEOUS;
    
    throw std::invalid_argument("Unknown overcurrent curve: " + str);
}

std::string OvercurrentTester::curveToString(OCCurve curve) {
    switch (curve) {
        case OCCurve::STANDARD_INVERSE: return "STANDARD_INVERSE";
        case OCCurve::VERY_INVERSE: return "VERY_INVERSE";
        case OCCurve::EXTREMELY_INVERSE: return "EXTREMELY_INVERSE";
        case OCCurve::LONG_TIME_INVERSE: return "LONG_TIME_INVERSE";
        case OCCurve::IEEE_MODERATELY_INVERSE: return "IEEE_MODERATELY_INVERSE";
        case OCCurve::IEEE_VERY_INVERSE: return "IEEE_VERY_INVERSE";
        case OCCurve::IEEE_EXTREMELY_INVERSE: return "IEEE_EXTREMELY_INVERSE";
        case OCCurve::DEFINITE_TIME: return "DEFINITE_TIME";
        case OCCurve::INSTANTANEOUS: return "INSTANTANEOUS";
        default: return "UNKNOWN";
    }
}

double OvercurrentTester::calculateIDMT(OCCurve curve, double TMS, double M) {
    // M = I / Ipickup (current multiple)
    
    if (M <= 1.0) {
        return std::numeric_limits<double>::infinity(); // No trip below pickup
    }
    
    double k, alpha, c;
    
    // IEC curves: t = TMS × k / ((M^alpha) - 1) + c
    switch (curve) {
        case OCCurve::STANDARD_INVERSE:
            k = 0.14;
            alpha = 0.02;
            c = 0.0;
            break;
            
        case OCCurve::VERY_INVERSE:
            k = 13.5;
            alpha = 1.0;
            c = 0.0;
            break;
            
        case OCCurve::EXTREMELY_INVERSE:
            k = 80.0;
            alpha = 2.0;
            c = 0.0;
            break;
            
        case OCCurve::LONG_TIME_INVERSE:
            k = 120.0;
            alpha = 1.0;
            c = 0.0;
            break;
            
        // IEEE curves: t = TMS × [A / (M^p - 1) + B]
        case OCCurve::IEEE_MODERATELY_INVERSE:
            // t = TMS × [0.0515 / (M^0.02 - 1) + 0.114]
            return TMS * (0.0515 / (std::pow(M, 0.02) - 1.0) + 0.114);
            
        case OCCurve::IEEE_VERY_INVERSE:
            // t = TMS × [19.61 / (M^2 - 1) + 0.491]
            return TMS * (19.61 / (std::pow(M, 2.0) - 1.0) + 0.491);
            
        case OCCurve::IEEE_EXTREMELY_INVERSE:
            // t = TMS × [28.2 / (M^2 - 1) + 0.1217]
            return TMS * (28.2 / (std::pow(M, 2.0) - 1.0) + 0.1217);
            
        case OCCurve::DEFINITE_TIME:
            return TMS; // Constant time
            
        case OCCurve::INSTANTANEOUS:
            return 0.0; // No intentional delay
            
        default:
            return std::numeric_limits<double>::infinity();
    }
    
    // IEC formula
    return TMS * (k / (std::pow(M, alpha) - 1.0)) + c;
}

double OvercurrentTester::calculateTripTime(const OCSettings& settings, double currentMultiple) {
    return calculateIDMT(settings.curve, settings.TMS, currentMultiple);
}

bool OvercurrentTester::waitWithStopCheck(double duration) {
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

bool OvercurrentTester::monitorTrip(double maxDuration, double& tripTime) {
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

OCResult OvercurrentTester::testPoint(const OCPoint& point, const OCTestConfig& config) {
    OCResult result;
    result.currentMultiple = point.currentMultiple;
    result.actualCurrent = config.settings.pickupCurrent * point.currentMultiple;
    result.tripped = false;
    result.measuredTime = 0.0;
    result.expectedTime = point.expectedTime;
    result.passed = false;
    
    try {
        // Set the current
        currentSetter_(result.actualCurrent);
        
        // Monitor for trip
        double tripTime = 0.0;
        bool tripped = monitorTrip(config.maxTestDuration, tripTime);
        
        result.tripped = tripped;
        result.measuredTime = tripTime;
        
        // Check if result matches expected
        if (tripped) {
            double tolerance;
            if (config.toleranceIsPercent) {
                tolerance = result.expectedTime * (config.timeTolerance / 100.0);
            } else {
                tolerance = config.timeTolerance;
            }
            
            double timeDiff = std::abs(result.measuredTime - result.expectedTime);
            result.passed = (timeDiff <= tolerance);
            
            if (!result.passed) {
                result.error = "Trip time outside tolerance";
            }
        } else {
            result.error = "Relay did not trip within max test duration";
            result.passed = false;
        }
        
    } catch (const std::exception& e) {
        result.error = std::string("Exception: ") + e.what();
        result.passed = false;
    }
    
    return result;
}

std::vector<OCResult> OvercurrentTester::run(const OCTestConfig& config,
                                              OCProgressCallback progressCallback) {
    std::vector<OCResult> results;
    
    if (running_) {
        OCResult errorResult;
        errorResult.error = "Test already running";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (!tripFlagGetter_) {
        OCResult errorResult;
        errorResult.error = "TRIP_FLAG getter not configured";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (!currentSetter_) {
        OCResult errorResult;
        errorResult.error = "Current setter not configured";
        errorResult.passed = false;
        results.push_back(errorResult);
        return results;
    }
    
    if (config.points.empty()) {
        OCResult errorResult;
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
            OCResult errorResult;
            errorResult.error = "Test stopped by user";
            errorResult.passed = false;
            results.push_back(errorResult);
            break;
        }
        
        const OCPoint& point = config.points[i];
        
        // Progress callback
        if (progressCallback) {
            progressCallback(static_cast<int>(i), static_cast<int>(config.points.size()), point);
        }
        
        // Test the point
        OCResult result = testPoint(point, config);
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
