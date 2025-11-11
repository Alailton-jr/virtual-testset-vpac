#include <gtest/gtest.h>
#include "overcurrent_tester.hpp"
#include <cmath>
#include <thread>
#include <chrono>

using vts::testers::OvercurrentTester;
using vts::testers::OCCurve;
using vts::testers::OCSettings;
using vts::testers::OCPoint;
using vts::testers::OCResult;
using vts::testers::OCTestConfig;

class OvercurrentTesterTest : public ::testing::Test {
protected:
    void SetUp() override {
        tester = std::make_unique<OvercurrentTester>();
        tripFlag = 0;
        currentCurrent = 0.0;
    }

    void TearDown() override {
        tester.reset();
    }

    std::unique_ptr<OvercurrentTester> tester;
    int tripFlag;
    double currentCurrent;
};

// Test 1: Curve parsing
TEST_F(OvercurrentTesterTest, ParseCurve) {
    EXPECT_EQ(tester->parseCurve("STANDARD_INVERSE"), OCCurve::STANDARD_INVERSE);
    EXPECT_EQ(tester->parseCurve("VERY_INVERSE"), OCCurve::VERY_INVERSE);
    EXPECT_EQ(tester->parseCurve("EXTREMELY_INVERSE"), OCCurve::EXTREMELY_INVERSE);
    EXPECT_EQ(tester->parseCurve("LONG_TIME_INVERSE"), OCCurve::LONG_TIME_INVERSE);
    EXPECT_EQ(tester->parseCurve("IEEE_MODERATELY_INVERSE"), OCCurve::IEEE_MODERATELY_INVERSE);
    EXPECT_EQ(tester->parseCurve("IEEE_VERY_INVERSE"), OCCurve::IEEE_VERY_INVERSE);
    EXPECT_EQ(tester->parseCurve("IEEE_EXTREMELY_INVERSE"), OCCurve::IEEE_EXTREMELY_INVERSE);
    EXPECT_EQ(tester->parseCurve("DEFINITE_TIME"), OCCurve::DEFINITE_TIME);
    EXPECT_EQ(tester->parseCurve("INSTANTANEOUS"), OCCurve::INSTANTANEOUS);
    
    EXPECT_THROW(tester->parseCurve("INVALID"), std::invalid_argument);
}

// Test 2: Curve to string
TEST_F(OvercurrentTesterTest, CurveToString) {
    EXPECT_EQ(tester->curveToString(OCCurve::STANDARD_INVERSE), "STANDARD_INVERSE");
    EXPECT_EQ(tester->curveToString(OCCurve::IEEE_VERY_INVERSE), "IEEE_VERY_INVERSE");
}

// Test 3: IEC Standard Inverse calculation
TEST_F(OvercurrentTesterTest, IECStandardInverseCalculation) {
    OCSettings settings;
    settings.pickupCurrent = 100.0;
    settings.TMS = 0.1;
    settings.curve = OCCurve::STANDARD_INVERSE;
    
    // At 2x pickup (M=2)
    double time = OvercurrentTester::calculateTripTime(settings, 2.0);
    
    // IEC SI formula: t = TMS * 0.14 / ((M^0.02) - 1)
    // t = 0.1 * 0.14 / ((2^0.02) - 1)
    // 2^0.02 ≈ 1.0140, so (2^0.02 - 1) ≈ 0.0140
    // t = 0.1 * 0.14 / 0.0140 ≈ 0.1 * 10 = 1.0 seconds
    EXPECT_NEAR(time, 1.0, 0.1);
}

// Test 4: IEEE Moderately Inverse calculation
TEST_F(OvercurrentTesterTest, IEEEModeratelyInverseCalculation) {
    OCSettings settings;
    settings.pickupCurrent = 100.0;
    settings.TMS = 0.5;
    settings.curve = OCCurve::IEEE_MODERATELY_INVERSE;
    
    // At 3x pickup
    double time = OvercurrentTester::calculateTripTime(settings, 3.0);
    
    // Should be positive and reasonable
    EXPECT_GT(time, 0.0);
    EXPECT_LT(time, 100.0);
}

// Test 5: Definite time calculation
TEST_F(OvercurrentTesterTest, DefiniteTimeCalculation) {
    OCSettings settings;
    settings.pickupCurrent = 100.0;
    settings.TMS = 1.0; // 1 second delay
    settings.curve = OCCurve::DEFINITE_TIME;
    
    // Should return TMS regardless of current multiple
    EXPECT_EQ(OvercurrentTester::calculateTripTime(settings, 2.0), 1.0);
    EXPECT_EQ(OvercurrentTester::calculateTripTime(settings, 5.0), 1.0);
    EXPECT_EQ(OvercurrentTester::calculateTripTime(settings, 10.0), 1.0);
}

// Test 6: Instantaneous trip
TEST_F(OvercurrentTesterTest, InstantaneousTripCalculation) {
    OCSettings settings;
    settings.pickupCurrent = 1000.0;
    settings.TMS = 0.0;
    settings.curve = OCCurve::INSTANTANEOUS;
    
    // Should return 0
    EXPECT_EQ(OvercurrentTester::calculateTripTime(settings, 10.0), 0.0);
}

// Test 7: Single point test with definite time
TEST_F(OvercurrentTesterTest, SinglePointTest) {
    OCTestConfig config;
    config.settings.pickupCurrent = 100.0;
    config.settings.TMS = 0.5;
    config.settings.curve = OCCurve::DEFINITE_TIME;
    config.timeTolerance = 0.1; // 100ms absolute
    config.toleranceIsPercent = false;
    config.maxTestDuration = 10.0;
    
    OCPoint point;
    point.currentMultiple = 2.0;
    point.expectedTime = 0.5;
    point.label = "2x pickup";
    config.points.push_back(point);
    
    tester->setTripFlagGetter([this]() { return tripFlag; });
    
    tester->setCurrentSetter([this, &config](double current) {
        currentCurrent = current;
        EXPECT_NEAR(current, 200.0, 1.0); // 2x100A
        
        // Simulate relay trip after 500ms in a background thread
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            tripFlag = 1;
        }).detach();
    });
    
    auto results = tester->run(config);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].tripped);
    EXPECT_NEAR(results[0].measuredTime, 0.5, 0.2);
    EXPECT_TRUE(results[0].passed);
}

// Test 8: Multiple current points
TEST_F(OvercurrentTesterTest, MultiplePoints) {
    OCTestConfig config;
    config.settings.pickupCurrent = 100.0;
    config.settings.TMS = 0.1;
    config.settings.curve = OCCurve::DEFINITE_TIME;
    config.timeTolerance = 10.0; // 10% tolerance
    config.toleranceIsPercent = true;
    config.maxTestDuration = 5.0;
    
    config.points.push_back({2.0, 0.1, "2x"});
    config.points.push_back({3.0, 0.1, "3x"});
    config.points.push_back({5.0, 0.1, "5x"});
    
    tester->setTripFlagGetter([this]() { return tripFlag; });
    
    int pointIndex = 0;
    tester->setCurrentSetter([this, &pointIndex](double current) {
        currentCurrent = current;
        pointIndex++;
        
        // Reset trip flag first
        tripFlag = 0;
        
        // Simulate trip after 100ms in background thread
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            tripFlag = 1;
        }).detach();
    });
    
    auto results = tester->run(config);
    
    EXPECT_EQ(results.size(), 3);
    
    for (const auto& result : results) {
        EXPECT_TRUE(result.tripped);
        EXPECT_NEAR(result.measuredTime, 0.1, 0.05);
    }
}

// Test 9: Current below pickup (should not trip)
TEST_F(OvercurrentTesterTest, BelowPickup) {
    OCTestConfig config;
    config.settings.pickupCurrent = 100.0;
    config.settings.TMS = 1.0;
    config.settings.curve = OCCurve::DEFINITE_TIME;
    config.timeTolerance = 0.1;
    config.toleranceIsPercent = false;
    config.maxTestDuration = 2.0;
    
    // 0.5x pickup - below threshold
    config.points.push_back({0.5, 0.0, "0.5x"});
    
    tester->setTripFlagGetter([this]() { return tripFlag; });
    
    tester->setCurrentSetter([this](double current) {
        currentCurrent = current;
        // Should NOT trip
        tripFlag = 0;
    });
    
    auto results = tester->run(config);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_FALSE(results[0].tripped);
}

// Test 10: Percentage tolerance check
TEST_F(OvercurrentTesterTest, PercentageTolerance) {
    OCTestConfig config;
    config.settings.pickupCurrent = 100.0;
    config.settings.TMS = 1.0;
    config.settings.curve = OCCurve::DEFINITE_TIME;
    config.timeTolerance = 20.0; // 20%
    config.toleranceIsPercent = true;
    config.maxTestDuration = 5.0;
    
    config.points.push_back({2.0, 1.0, "2x"});
    
    tester->setTripFlagGetter([this]() { return tripFlag; });
    
    tester->setCurrentSetter([this](double current) {
        // Trip at 1.15 seconds (15% over expected 1.0s) in background thread
        tripFlag = 0;  // Reset first
        std::thread([this]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1150));
            tripFlag = 1;
        }).detach();
    });
    
    auto results = tester->run(config);
    
    EXPECT_EQ(results.size(), 1);
    EXPECT_TRUE(results[0].passed); // Within 20% tolerance
}

// Test 11: IEC Very Inverse curve
TEST_F(OvercurrentTesterTest, IECVeryInverseCalculation) {
    OCSettings settings;
    settings.pickupCurrent = 100.0;
    settings.TMS = 0.1;
    settings.curve = OCCurve::VERY_INVERSE;
    
    // At 5x pickup
    double time = OvercurrentTester::calculateTripTime(settings, 5.0);
    
    // Should be a reasonable time
    EXPECT_GT(time, 0.0);
    EXPECT_LT(time, 10.0);
}

// Test 12: IEC Extremely Inverse curve  
TEST_F(OvercurrentTesterTest, IECExtremelyInverseCalculation) {
    OCSettings settings;
    settings.pickupCurrent = 100.0;
    settings.TMS = 0.1;
    settings.curve = OCCurve::EXTREMELY_INVERSE;
    
    // At 10x pickup
    double time = OvercurrentTester::calculateTripTime(settings, 10.0);
    
    EXPECT_GT(time, 0.0);
    EXPECT_LT(time, 5.0);
}
