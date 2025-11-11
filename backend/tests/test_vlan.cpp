/**
 * @file test_vlan.cpp
 * @brief Unit tests for VLAN parameter validation (Phase 1.8)
 * 
 * Tests cover:
 * - Priority validation (0-7, 3 bits)
 * - VLAN ID validation (0-4095, 12 bits)
 * - Invalid parameters rejection
 */

#include <gtest/gtest.h>
#include "Virtual_LAN.hpp"
#include <stdexcept>

// Test fixture for VLAN tests
class VLANTest : public ::testing::Test {
protected:
    // Helper to create VLAN and check if it throws
    // Virtual_LAN constructor signature: Virtual_LAN(priority, dei, id)
    void ExpectValidVLAN(uint8_t priority, bool dei, uint16_t id) {
        EXPECT_NO_THROW({
            Virtual_LAN vlan(priority, dei, id);
        });
    }
    
    void ExpectInvalidVLAN(uint8_t priority, bool dei, uint16_t id) {
        EXPECT_THROW({
            Virtual_LAN vlan(priority, dei, id);
        }, std::invalid_argument);
    }
};

// Test valid priority values (0-7)
TEST_F(VLANTest, ValidPriority_0) {
    ExpectValidVLAN(0, false, 100);  // priority=0, dei=false, id=100
}

TEST_F(VLANTest, ValidPriority_7) {
    ExpectValidVLAN(7, false, 100);  // priority=7, dei=false, id=100
}

TEST_F(VLANTest, ValidPriority_All) {
    for (uint8_t prio = 0; prio <= 7; ++prio) {
        ExpectValidVLAN(prio, false, 100);  // All priorities with id=100
    }
}

// Test invalid priority values (>7)
TEST_F(VLANTest, InvalidPriority_8) {
    ExpectInvalidVLAN(8, false, 100);
}

TEST_F(VLANTest, InvalidPriority_15) {
    ExpectInvalidVLAN(15, false, 100);
}

TEST_F(VLANTest, InvalidPriority_255) {
    ExpectInvalidVLAN(255, false, 100);
}

// Test valid VLAN IDs (0-4095)
TEST_F(VLANTest, ValidVLAN_0) {
    ExpectValidVLAN(4, false, 0);  // priority=4, dei=false, id=0
}

TEST_F(VLANTest, ValidVLAN_1) {
    ExpectValidVLAN(4, false, 1);  // priority=4, dei=false, id=1
}

TEST_F(VLANTest, ValidVLAN_100) {
    ExpectValidVLAN(4, false, 100);  // priority=4, dei=false, id=100
}

TEST_F(VLANTest, ValidVLAN_4095) {
    ExpectValidVLAN(4, false, 4095);  // priority=4, dei=false, id=4095 (max valid)
}

// Test DEI values (0-1)
TEST_F(VLANTest, ValidDEI_0) {
    ExpectValidVLAN(4, false, 100);  // priority=4, dei=false, id=100
}

TEST_F(VLANTest, ValidDEI_1) {
    ExpectValidVLAN(4, true, 100);  // priority=4, dei=true, id=100
}

TEST_F(VLANTest, InvalidDEI_2) {
    // Note: DEI is bool type, so any non-zero value becomes true
    // Cannot test invalid DEI values through constructor
    EXPECT_NO_THROW({
        Virtual_LAN vlan(4, true, 100);  // DEI is bool, any value works
    });
}

// Test getEncoded produces correct TCI (Tag Control Information)
TEST_F(VLANTest, EncodedTCI_Calculation) {
    Virtual_LAN vlan(4, false, 100);  // priority=4, dei=false, id=100
    auto encoded = vlan.getEncoded();
    
    // VLAN tag: [0x81, 0x00, TCI_high, TCI_low] (4 bytes, no EtherType)
    ASSERT_EQ(encoded.size(), 4);
    EXPECT_EQ(encoded[0], 0x81);
    EXPECT_EQ(encoded[1], 0x00);
    
    // TCI = (priority << 13) | (dei << 12) | vlan_id
    // priority=4, dei=0, vlan_id=100
    // TCI = (4 << 13) | (0 << 12) | 100 = 32768 | 0 | 100 = 32868 = 0x8064
    uint16_t tci = static_cast<uint16_t>((static_cast<uint16_t>(encoded[2]) << 8) | encoded[3]);
    EXPECT_EQ(tci, 0x8064);
}

// Test boundary conditions
TEST_F(VLANTest, BoundaryConditions) {
    // Min values
    ExpectValidVLAN(0, false, 0);  // priority=0, dei=false, id=0
    
    // Max values
    ExpectValidVLAN(7, true, 4095);  // priority=7, dei=true, id=4095 (all max valid)
    
    // Just over boundary - priority
    ExpectInvalidVLAN(8, false, 0);   // Priority > 7
    
    // VLAN ID over boundary requires direct test (not through helper)
    EXPECT_THROW({
        Virtual_LAN vlan(0, false, 4096);  // VLAN ID = 4096 > 4095
    }, std::invalid_argument);
}
