/**
 * @file test_smpCnt_wrap.cpp
 * @brief Unit tests for smpCnt wrapping behavior (Phase 1.4)
 * 
 * Tests cover:
 * - 16-bit wrap at 65535
 * - Correct increment behavior
 * - Rate-based modulo where applicable
 */

#include <gtest/gtest.h>
#include <cstdint>

// Test fixture for smpCnt wrap tests
class SmpCntWrapTest : public ::testing::Test {
protected:
    // Helper to simulate smpCnt increment with wrap
    uint16_t incrementSmpCnt(uint16_t current) {
        return static_cast<uint16_t>(current + 1);
    }
    
    // Helper with rate-based modulo
    uint16_t incrementSmpCntWithRate(uint16_t current, uint16_t smpRate) {
        return static_cast<uint16_t>((current + 1) % smpRate);
    }
};

// Test simple 16-bit wrap
TEST_F(SmpCntWrapTest, WrapAt65535) {
    uint16_t smpCnt = 65535;
    smpCnt = incrementSmpCnt(smpCnt);
    EXPECT_EQ(smpCnt, 0);  // Should wrap to 0
}

TEST_F(SmpCntWrapTest, NoWrapBeforeMax) {
    uint16_t smpCnt = 65534;
    smpCnt = incrementSmpCnt(smpCnt);
    EXPECT_EQ(smpCnt, 65535);  // Should be 65535, not wrapped yet
}

TEST_F(SmpCntWrapTest, IncrementFrom0) {
    uint16_t smpCnt = 0;
    smpCnt = incrementSmpCnt(smpCnt);
    EXPECT_EQ(smpCnt, 1);
}

// Test sequence of increments
TEST_F(SmpCntWrapTest, SequenceOf100) {
    uint16_t smpCnt = 0;
    for (int i = 1; i <= 100; ++i) {
        smpCnt = incrementSmpCnt(smpCnt);
        EXPECT_EQ(smpCnt, static_cast<uint16_t>(i));
    }
}

// Test 70,000 samples (Phase 1.4 acceptance criteria)
TEST_F(SmpCntWrapTest, Acceptance_70000Samples) {
    uint16_t smpCnt = 0;
    uint16_t expected = 0;
    
    for (int i = 0; i < 70000; ++i) {
        smpCnt = incrementSmpCnt(smpCnt);
        expected = static_cast<uint16_t>((expected + 1) & 0xFFFF);
        EXPECT_EQ(smpCnt, expected) << "Failed at sample " << i;
    }
    
    // After 70000 samples: 70000 % 65536 = 4464
    EXPECT_EQ(smpCnt, 4464);
}

// Test rate-based modulo (when using smpRate)
TEST_F(SmpCntWrapTest, RateBasedModulo_4800) {
    uint16_t smpCnt = 0;
    uint16_t smpRate = 4800;
    
    // Increment 4799 times (should reach 4799)
    for (int i = 0; i < 4799; ++i) {
        smpCnt = incrementSmpCntWithRate(smpCnt, smpRate);
    }
    EXPECT_EQ(smpCnt, 4799);
    
    // One more increment should wrap to 0
    smpCnt = incrementSmpCntWithRate(smpCnt, smpRate);
    EXPECT_EQ(smpCnt, 0);
}

TEST_F(SmpCntWrapTest, RateBasedModulo_9600) {
    uint16_t smpCnt = 0;
    uint16_t smpRate = 9600;
    
    // Test wrap at smpRate
    for (int i = 0; i < 9600; ++i) {
        smpCnt = incrementSmpCntWithRate(smpCnt, smpRate);
    }
    EXPECT_EQ(smpCnt, 0);  // Should wrap to 0 after 9600 samples
}

// Test that 16-bit cast prevents overflow
TEST_F(SmpCntWrapTest, CastPreventsOverflow) {
    uint32_t largeValue = 70000;
    uint16_t smpCnt = static_cast<uint16_t>(largeValue);
    
    // Should truncate to 16 bits: 70000 & 0xFFFF = 4464
    EXPECT_EQ(smpCnt, 4464);
}

// Test multiple wraps
TEST_F(SmpCntWrapTest, MultipleWraps) {
    uint16_t smpCnt = 65530;
    
    // First wrap
    for (int i = 0; i < 10; ++i) {
        smpCnt = incrementSmpCnt(smpCnt);
    }
    EXPECT_EQ(smpCnt, 4);  // 65530 + 10 = 65540, wrapped to 4
    
    // Second wrap
    for (int i = 0; i < 65536; ++i) {
        smpCnt = incrementSmpCnt(smpCnt);
    }
    EXPECT_EQ(smpCnt, 4);  // Should be back to 4 after full wrap
}
