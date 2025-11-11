#include <gtest/gtest.h>
#include "sequence_engine.hpp"
#include "global_flags.hpp"

#include <thread>
#include <chrono>

using namespace vts::sequence;
using namespace std::chrono_literals;

class SequenceEngineTest : public ::testing::Test {
protected:
    SequenceEngine engine;
    
    // Track progress callbacks
    size_t progressCallCount = 0;
    size_t lastCurrentState = 0;
    size_t lastTotalStates = 0;
    std::string lastStateName;
    std::string lastMessage;
    
    // Track phasor updates
    size_t phasorUpdateCount = 0;
    std::string lastStreamId;
    StreamPhasorState lastPhasorState;
    
    void SetUp() override {
        vts::clearTripFlag();
        progressCallCount = 0;
        phasorUpdateCount = 0;
        
        // Set up callbacks
        engine.setProgressCallback([this](size_t current, size_t total, 
                                          const std::string& name, double elapsed,
                                          const std::string& msg) {
            progressCallCount++;
            lastCurrentState = current;
            lastTotalStates = total;
            lastStateName = name;
            lastMessage = msg;
        });
        
        engine.setPhasorUpdateCallback([this](const std::string& streamId,
                                              const StreamPhasorState& state) {
            phasorUpdateCount++;
            lastStreamId = streamId;
            lastPhasorState = state;
        });
    }
    
    void TearDown() override {
        engine.stop();
        std::this_thread::sleep_for(100ms);
    }
    
    Sequence createSimpleSequence(int numStates = 2, double durationSec = 0.1) {
        Sequence seq;
        seq.activeStreams = {"SV1"};
        
        for (int i = 0; i < numStates; i++) {
            SequenceState state;
            state.name = "State" + std::to_string(i + 1);
            state.durationSec = durationSec;
            state.transition = StateTransition(TransitionType::TIME);
            
            StreamPhasorState phasors;
            phasors.freq = 60.0 + i;  // Different freq per state
            phasors.channels["V-A"] = ChannelPhasor(69000.0, 0.0);
            
            state.phasors["SV1"] = phasors;
            seq.states.push_back(state);
        }
        
        return seq;
    }
};

// Test 1: Start and complete simple sequence
TEST_F(SequenceEngineTest, StartAndCompleteSimpleSequence) {
    Sequence seq = createSimpleSequence(2, 0.1);
    
    EXPECT_TRUE(engine.start(seq));
    EXPECT_EQ(engine.getStatus(), SequenceStatus::RUNNING);
    
    // Wait for completion
    std::this_thread::sleep_for(300ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
    EXPECT_GT(progressCallCount, 0);
    EXPECT_EQ(phasorUpdateCount, 2);  // 2 states
}

// Test 2: Cannot start when already running
TEST_F(SequenceEngineTest, CannotStartWhenRunning) {
    Sequence seq = createSimpleSequence(2, 0.1);
    
    EXPECT_TRUE(engine.start(seq));
    EXPECT_FALSE(engine.start(seq));  // Should fail
    
    std::string error = engine.getLastError();
    EXPECT_FALSE(error.empty());
    EXPECT_NE(error.find("already running"), std::string::npos);
    
    std::this_thread::sleep_for(300ms);
}

// Test 3: Stop sequence mid-execution
TEST_F(SequenceEngineTest, StopSequence) {
    Sequence seq = createSimpleSequence(3, 1.0);  // Long duration
    
    EXPECT_TRUE(engine.start(seq));
    EXPECT_EQ(engine.getStatus(), SequenceStatus::RUNNING);
    
    std::this_thread::sleep_for(100ms);
    
    engine.stop();
    std::this_thread::sleep_for(100ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::STOPPED);
}

// Test 4: Pause and resume sequence
TEST_F(SequenceEngineTest, PauseAndResume) {
    Sequence seq = createSimpleSequence(3, 0.2);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(100ms);
    
    engine.pause();
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(engine.getStatus(), SequenceStatus::PAUSED);
    
    int stateBeforePause = engine.getCurrentStateIndex();
    
    std::this_thread::sleep_for(200ms);  // Wait while paused
    
    // Should still be in same state
    EXPECT_EQ(engine.getCurrentStateIndex(), stateBeforePause);
    
    engine.resume();
    EXPECT_EQ(engine.getStatus(), SequenceStatus::RUNNING);
    
    std::this_thread::sleep_for(700ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
}

// Test 5: GOOSE trip transition
TEST_F(SequenceEngineTest, GooseTripTransition) {
    Sequence seq;
    seq.activeStreams = {"SV1"};
    
    // State 1: Short time transition
    SequenceState state1;
    state1.name = "Pre-Fault";
    state1.durationSec = 0.1;
    state1.transition = StateTransition(TransitionType::TIME);
    state1.phasors["SV1"].freq = 60.0;
    
    // State 2: GOOSE trip transition
    SequenceState state2;
    state2.name = "Fault";
    state2.durationSec = 2.0;  // Long timeout, but trip will happen sooner
    state2.transition = StateTransition(TransitionType::GOOSE_TRIP);
    state2.phasors["SV1"].freq = 60.5;
    
    seq.states.push_back(state1);
    seq.states.push_back(state2);
    
    EXPECT_TRUE(engine.start(seq));
    
    // Wait for first state
    std::this_thread::sleep_for(150ms);
    
    EXPECT_EQ(engine.getCurrentStateIndex(), 1);  // In second state
    
    // Trigger trip
    vts::setTripFlag();
    
    // Wait for trip detection
    std::this_thread::sleep_for(200ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
}

// Test 6: GOOSE trip timeout
TEST_F(SequenceEngineTest, GooseTripTimeout) {
    Sequence seq;
    seq.activeStreams = {"SV1"};
    
    SequenceState state;
    state.name = "WaitForTrip";
    state.durationSec = 0.2;  // Short timeout
    state.transition = StateTransition(TransitionType::GOOSE_TRIP);
    state.phasors["SV1"].freq = 60.0;
    
    seq.states.push_back(state);
    
    EXPECT_TRUE(engine.start(seq));
    
    // Don't set trip flag - should timeout
    std::this_thread::sleep_for(300ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
}

// Test 7: Multiple streams
TEST_F(SequenceEngineTest, MultipleStreams) {
    Sequence seq;
    seq.activeStreams = {"SV1", "SV2", "SV3"};
    
    SequenceState state;
    state.name = "MultiStream";
    state.durationSec = 0.1;
    state.transition = StateTransition(TransitionType::TIME);
    
    for (const auto& streamId : seq.activeStreams) {
        StreamPhasorState phasors;
        phasors.freq = 60.0;
        phasors.channels["V-A"] = ChannelPhasor(69000.0, 0.0);
        state.phasors[streamId] = phasors;
    }
    
    seq.states.push_back(state);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(200ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
    EXPECT_EQ(phasorUpdateCount, 3);  // 3 streams updated
}

// Test 8: State timing
TEST_F(SequenceEngineTest, StateTiming) {
    Sequence seq = createSimpleSequence(1, 0.2);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(100ms);
    
    double elapsed = engine.getStateElapsedTime();
    EXPECT_GE(elapsed, 0.09);  // At least 90ms
    EXPECT_LE(elapsed, 0.15);  // At most 150ms (some slack)
    
    std::this_thread::sleep_for(200ms);
}

// Test 9: Total elapsed time
TEST_F(SequenceEngineTest, TotalElapsedTime) {
    Sequence seq = createSimpleSequence(2, 0.1);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(150ms);
    
    double totalElapsed = engine.getTotalElapsedTime();
    EXPECT_GE(totalElapsed, 0.14);  // At least 140ms
    EXPECT_LE(totalElapsed, 0.20);  // At most 200ms
    
    std::this_thread::sleep_for(200ms);
}

// Test 10: Current state index tracking
TEST_F(SequenceEngineTest, CurrentStateIndex) {
    Sequence seq = createSimpleSequence(3, 0.1);
    
    EXPECT_EQ(engine.getCurrentStateIndex(), -1);  // Not running
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(50ms);
    EXPECT_EQ(engine.getCurrentStateIndex(), 0);
    
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(engine.getCurrentStateIndex(), 1);
    
    std::this_thread::sleep_for(100ms);
    EXPECT_EQ(engine.getCurrentStateIndex(), 2);
    
    std::this_thread::sleep_for(150ms);
    EXPECT_EQ(engine.getCurrentStateIndex(), -1);  // Completed
}

// Test 11: Empty sequence validation
TEST_F(SequenceEngineTest, EmptySequenceValidation) {
    Sequence seq;
    seq.activeStreams = {"SV1"};
    // No states
    
    EXPECT_FALSE(engine.start(seq));
    
    std::string error = engine.getLastError();
    EXPECT_NE(error.find("no states"), std::string::npos);
}

// Test 12: No active streams validation
TEST_F(SequenceEngineTest, NoActiveStreamsValidation) {
    Sequence seq = createSimpleSequence(2, 0.1);
    seq.activeStreams.clear();
    
    EXPECT_FALSE(engine.start(seq));
    
    std::string error = engine.getLastError();
    EXPECT_NE(error.find("no active streams"), std::string::npos);
}

// Test 13: Progress callback receives correct data
TEST_F(SequenceEngineTest, ProgressCallbackData) {
    Sequence seq = createSimpleSequence(2, 0.1);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(50ms);
    
    EXPECT_GT(progressCallCount, 0);
    EXPECT_EQ(lastTotalStates, 2);
    
    std::this_thread::sleep_for(300ms);
}

// Test 14: Phasor update callback receives correct data
TEST_F(SequenceEngineTest, PhasorUpdateCallbackData) {
    Sequence seq;
    seq.activeStreams = {"TEST_STREAM"};
    
    SequenceState state;
    state.name = "TestState";
    state.durationSec = 0.1;
    state.transition = StateTransition(TransitionType::TIME);
    
    StreamPhasorState phasors;
    phasors.freq = 50.0;
    phasors.channels["V-A"] = ChannelPhasor(110000.0, -30.0);
    phasors.channels["I-A"] = ChannelPhasor(1000.0, 45.0);
    
    state.phasors["TEST_STREAM"] = phasors;
    seq.states.push_back(state);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(50ms);
    
    EXPECT_EQ(phasorUpdateCount, 1);
    EXPECT_EQ(lastStreamId, "TEST_STREAM");
    EXPECT_DOUBLE_EQ(lastPhasorState.freq, 50.0);
    EXPECT_EQ(lastPhasorState.channels.size(), 2);
    EXPECT_DOUBLE_EQ(lastPhasorState.channels["V-A"].mag, 110000.0);
    EXPECT_DOUBLE_EQ(lastPhasorState.channels["V-A"].angleDeg, -30.0);
    
    std::this_thread::sleep_for(200ms);
}

// Test 15: Multiple channel phasors
TEST_F(SequenceEngineTest, MultipleChannelPhasors) {
    Sequence seq;
    seq.activeStreams = {"SV1"};
    
    SequenceState state;
    state.name = "MultiChannel";
    state.durationSec = 0.1;
    state.transition = StateTransition(TransitionType::TIME);
    
    StreamPhasorState phasors;
    phasors.freq = 60.0;
    phasors.channels["V-A"] = ChannelPhasor(69000.0, 0.0);
    phasors.channels["V-B"] = ChannelPhasor(69000.0, -120.0);
    phasors.channels["V-C"] = ChannelPhasor(69000.0, 120.0);
    phasors.channels["I-A"] = ChannelPhasor(1000.0, -30.0);
    phasors.channels["I-B"] = ChannelPhasor(1000.0, -150.0);
    phasors.channels["I-C"] = ChannelPhasor(1000.0, 90.0);
    
    state.phasors["SV1"] = phasors;
    seq.states.push_back(state);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(50ms);
    
    EXPECT_EQ(lastPhasorState.channels.size(), 6);
    
    std::this_thread::sleep_for(200ms);
}

// Test 16: Stop during pause
TEST_F(SequenceEngineTest, StopDuringPause) {
    Sequence seq = createSimpleSequence(3, 0.5);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(100ms);
    
    engine.pause();
    std::this_thread::sleep_for(50ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::PAUSED);
    
    engine.stop();
    std::this_thread::sleep_for(100ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::STOPPED);
}

// Test 17: Status after completion
TEST_F(SequenceEngineTest, StatusAfterCompletion) {
    Sequence seq = createSimpleSequence(1, 0.1);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::IDLE);
    
    EXPECT_TRUE(engine.start(seq));
    EXPECT_EQ(engine.getStatus(), SequenceStatus::RUNNING);
    
    std::this_thread::sleep_for(200ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
}

// Test 18: Rapid GOOSE trip
TEST_F(SequenceEngineTest, RapidGooseTrip) {
    Sequence seq;
    seq.activeStreams = {"SV1"};
    
    SequenceState state;
    state.name = "QuickTrip";
    state.durationSec = 1.0;  // Long timeout
    state.transition = StateTransition(TransitionType::GOOSE_TRIP);
    state.phasors["SV1"].freq = 60.0;
    
    seq.states.push_back(state);
    
    EXPECT_TRUE(engine.start(seq));
    
    // Trip almost immediately
    std::this_thread::sleep_for(20ms);
    vts::setTripFlag();
    
    std::this_thread::sleep_for(100ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
}

// Test 19: Sequence with single state
TEST_F(SequenceEngineTest, SingleStateSequence) {
    Sequence seq = createSimpleSequence(1, 0.1);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(200ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
    EXPECT_EQ(phasorUpdateCount, 1);
}

// Test 20: Long sequence
TEST_F(SequenceEngineTest, LongSequence) {
    Sequence seq = createSimpleSequence(10, 0.05);
    
    EXPECT_TRUE(engine.start(seq));
    
    std::this_thread::sleep_for(600ms);
    
    EXPECT_EQ(engine.getStatus(), SequenceStatus::COMPLETED);
    EXPECT_EQ(phasorUpdateCount, 10);
}
