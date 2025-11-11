#include <gtest/gtest.h>
#include "comtrade_parser.hpp"
#include <fstream>
#include <cstdio>
#include <cmath>

using namespace vts::io;

class ComtradeParserTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test files in temp directory
        testDir_ = "/tmp/vts_comtrade_test/";
        system(("mkdir -p " + testDir_).c_str());
    }
    
    void TearDown() override {
        // Clean up test files
        system(("rm -rf " + testDir_).c_str());
    }
    
    void createTestCfgFile(const std::string& filename, bool includeTrigger = true) {
        std::ofstream file(testDir_ + filename);
        
        // Line 1: Station name, device ID, revision year
        file << "TEST_STATION,VTS_DEVICE,1999\n";
        
        // Line 2: Total channels, analog, digital
        file << "4,3A,1D\n";
        
        // Lines 3-5: Analog channels (index, name, phase, ccbm, units, a, b, skew, min, max, primary, secondary, ps)
        file << "1,VA,A,,kV,0.001,0,0,-200,200,115,1,P\n";
        file << "2,VB,B,,kV,0.001,0,0,-200,200,115,1,P\n";
        file << "3,IA,A,,A,0.01,0,0,-2000,2000,1000,1,P\n";
        
        // Line 6: Digital channel (index, name, phase, ccbm, normal_state)
        file << "1,TRIP,,,0\n";
        
        // Line 7: Line frequency
        file << "60\n";
        
        // Line 8: Number of sample rates
        file << "1\n";
        
        // Line 9: Sample rate, end sample
        file << "4800,100\n";
        
        // Line 10: Start date and time
        file << "01/01/2025,00:00:00.000000\n";
        
        // Line 11: Trigger date and time
        if (includeTrigger) {
            file << "01/01/2025,00:00:00.020833\n";
        } else {
            file << "01/01/2025,00:00:00.000000\n";
        }
        
        // Line 12: Data file type
        file << "ASCII\n";
        
        // Line 13: Time multiplication factor
        file << "1.0\n";
        
        file.close();
    }
    
    void createTestDatAsciiFile(const std::string& filename, int numSamples = 5) {
        std::ofstream file(testDir_ + filename);
        
        for (int i = 0; i < numSamples; i++) {
            // Sample format: sample#, timestamp_us, analog1, analog2, analog3, digital_word
            double t = i * (1000000.0 / 4800.0);  // microseconds
            
            // Simulate 60 Hz sine waves with different phases
            double angle = 2.0 * 3.14159265359 * 60.0 * (i / 4800.0);
            int32_t va = static_cast<int32_t>(100000.0 * sin(angle));
            int32_t vb = static_cast<int32_t>(100000.0 * sin(angle - 2.094395));  // -120 degrees
            int32_t ia = static_cast<int32_t>(50000.0 * sin(angle - 0.523599));   // -30 degrees
            
            uint32_t digital = (i >= 2) ? 1 : 0;  // TRIP asserts at sample 2
            
            file << i << "," << t << "," << va << "," << vb << "," << ia << "," << digital << "\n";
        }
        
        file.close();
    }
    
    void createTestCSVFile(const std::string& filename, int numSamples = 10) {
        std::ofstream file(testDir_ + filename);
        
        // Header
        file << "VA,VB,VC,IA,IB,IC\n";
        
        for (int i = 0; i < numSamples; i++) {
            double angle = 2.0 * 3.14159265359 * 60.0 * (i / 4800.0);
            double va = 100.0 * sin(angle);
            double vb = 100.0 * sin(angle - 2.094395);
            double vc = 100.0 * sin(angle + 2.094395);
            double ia = 50.0 * sin(angle - 0.523599);
            double ib = 50.0 * sin(angle - 2.618);
            double ic = 50.0 * sin(angle + 1.571);
            
            file << va << "," << vb << "," << vc << "," << ia << "," << ib << "," << ic << "\n";
        }
        
        file.close();
    }
    
    std::string testDir_;
};

// Test basic CFG parsing
TEST_F(ComtradeParserTest, ParseCfgBasic) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    ASSERT_TRUE(parser.isLoaded());
    
    const auto& config = parser.getConfig();
    
    EXPECT_EQ(config.stationName, "TEST_STATION");
    EXPECT_EQ(config.recDeviceId, "VTS_DEVICE");
    EXPECT_EQ(config.revisionYear, 1999);
    EXPECT_EQ(config.totalChannels, 4);
    EXPECT_EQ(config.numAnalogChannels, 3);
    EXPECT_EQ(config.numDigitalChannels, 1);
    EXPECT_DOUBLE_EQ(config.lineFreq, 60.0);
    EXPECT_EQ(config.numSampleRates, 1);
    EXPECT_DOUBLE_EQ(config.sampleRates[0].rate, 4800.0);
    EXPECT_EQ(config.dataFormat, DataFormat::ASCII);
}

// Test analog channel parsing
TEST_F(ComtradeParserTest, ParseAnalogChannels) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    const auto& config = parser.getConfig();
    ASSERT_EQ(config.analogChannels.size(), 3);
    
    // Check first channel (VA)
    const auto& ch0 = config.analogChannels[0];
    EXPECT_EQ(ch0.index, 0);  // 0-based internally
    EXPECT_EQ(ch0.name, "VA");
    EXPECT_EQ(ch0.phase, "A");
    EXPECT_EQ(ch0.units, "kV");
    EXPECT_DOUBLE_EQ(ch0.a, 0.001);
    EXPECT_DOUBLE_EQ(ch0.b, 0.0);
    
    // Check third channel (IA)
    const auto& ch2 = config.analogChannels[2];
    EXPECT_EQ(ch2.name, "IA");
    EXPECT_EQ(ch2.units, "A");
    EXPECT_DOUBLE_EQ(ch2.a, 0.01);
}

// Test digital channel parsing
TEST_F(ComtradeParserTest, ParseDigitalChannels) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    const auto& config = parser.getConfig();
    ASSERT_EQ(config.digitalChannels.size(), 1);
    
    const auto& ch0 = config.digitalChannels[0];
    EXPECT_EQ(ch0.index, 0);
    EXPECT_EQ(ch0.name, "TRIP");
    EXPECT_EQ(ch0.normalState, 0);
}

// Test ASCII DAT file parsing
TEST_F(ComtradeParserTest, ParseDatAscii) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    EXPECT_EQ(parser.getTotalSamples(), 5);
    
    // Get first sample
    ComtradeSample sample;
    ASSERT_TRUE(parser.getSample(0, sample));
    
    EXPECT_EQ(sample.sampleNumber, 0);
    EXPECT_EQ(sample.analogValues.size(), 3);
    EXPECT_EQ(sample.digitalValues.size(), 1);
    
    // Check digital value transitions
    ASSERT_TRUE(parser.getSample(1, sample));
    EXPECT_FALSE(sample.digitalValues[0]);  // TRIP = 0
    
    ASSERT_TRUE(parser.getSample(2, sample));
    EXPECT_TRUE(sample.digitalValues[0]);   // TRIP = 1
}

// Test scaling application
TEST_F(ComtradeParserTest, ScalingApplied) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    ComtradeSample sample;
    ASSERT_TRUE(parser.getSample(0, sample));
    
    // Raw value in .dat is ~100000, scaling factor a=0.001, b=0
    // So scaled value should be ~100.0 kV
    EXPECT_NEAR(sample.analogValues[0], 0.0, 150.0);  // Peak should be within range
    
    // Third channel (IA) has a=0.01
    EXPECT_NEAR(sample.analogValues[2], 0.0, 600.0);
}

// Test CSV loading
TEST_F(ComtradeParserTest, LoadCSV) {
    createTestCSVFile("test.csv", 10);
    
    ComtradeParser parser;
    std::vector<std::string> channelNames = {"VA", "VB", "VC", "IA", "IB", "IC"};
    
    ASSERT_TRUE(parser.loadCSV(testDir_ + "test.csv", 4800.0, channelNames));
    ASSERT_TRUE(parser.isLoaded());
    
    EXPECT_EQ(parser.getTotalSamples(), 10);
    
    const auto& config = parser.getConfig();
    EXPECT_EQ(config.numAnalogChannels, 6);
    EXPECT_EQ(config.analogChannels[0].name, "VA");
    EXPECT_EQ(config.analogChannels[5].name, "IC");
    EXPECT_DOUBLE_EQ(config.sampleRates[0].rate, 4800.0);
}

// Test CSV with scaling factors
TEST_F(ComtradeParserTest, LoadCSVWithScaling) {
    createTestCSVFile("test.csv", 10);
    
    ComtradeParser parser;
    std::vector<std::string> channelNames = {"VA", "VB"};
    std::vector<double> scaling = {2.0, 10.0, 0.5, 5.0};  // a,b pairs for 2 channels
    
    ASSERT_TRUE(parser.loadCSV(testDir_ + "test.csv", 4800.0, channelNames, scaling));
    
    const auto& config = parser.getConfig();
    EXPECT_DOUBLE_EQ(config.analogChannels[0].a, 2.0);
    EXPECT_DOUBLE_EQ(config.analogChannels[0].b, 10.0);
    EXPECT_DOUBLE_EQ(config.analogChannels[1].a, 0.5);
    EXPECT_DOUBLE_EQ(config.analogChannels[1].b, 5.0);
}

// Test sample retrieval
TEST_F(ComtradeParserTest, GetSample) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 10);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    ComtradeSample sample;
    
    // Valid indices
    EXPECT_TRUE(parser.getSample(0, sample));
    EXPECT_TRUE(parser.getSample(5, sample));
    EXPECT_TRUE(parser.getSample(9, sample));
    
    // Invalid indices
    EXPECT_FALSE(parser.getSample(-1, sample));
    EXPECT_FALSE(parser.getSample(10, sample));
    EXPECT_FALSE(parser.getSample(100, sample));
}

// Test channel lookup by name
TEST_F(ComtradeParserTest, GetChannelByName) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    const AnalogChannel* va = parser.getAnalogChannel("VA");
    ASSERT_NE(va, nullptr);
    EXPECT_EQ(va->name, "VA");
    EXPECT_EQ(va->phase, "A");
    
    const AnalogChannel* notFound = parser.getAnalogChannel("VX");
    EXPECT_EQ(notFound, nullptr);
    
    const DigitalChannel* trip = parser.getDigitalChannel("TRIP");
    ASSERT_NE(trip, nullptr);
    EXPECT_EQ(trip->name, "TRIP");
}

// Test getAllSamples
TEST_F(ComtradeParserTest, GetAllSamples) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 10);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    auto allSamples = parser.getAllSamples();
    EXPECT_EQ(allSamples.size(), 10);
    
    // Verify sample numbers are sequential
    for (size_t i = 0; i < allSamples.size(); i++) {
        EXPECT_EQ(allSamples[i].sampleNumber, static_cast<int>(i));
    }
}

// Test error handling - missing files
TEST_F(ComtradeParserTest, ErrorHandling_MissingFiles) {
    ComtradeParser parser;
    
    EXPECT_FALSE(parser.load(testDir_ + "nonexistent.cfg"));
    EXPECT_FALSE(parser.isLoaded());
    EXPECT_FALSE(parser.getLastError().empty());
}

// Test error handling - malformed CFG
TEST_F(ComtradeParserTest, ErrorHandling_MalformedCfg) {
    std::ofstream file(testDir_ + "bad.cfg");
    file << "BAD DATA\n";
    file << "NOT ENOUGH FIELDS\n";
    file.close();
    
    ComtradeParser parser;
    EXPECT_FALSE(parser.load(testDir_ + "bad.cfg"));
    EXPECT_FALSE(parser.isLoaded());
}

// Test clear() functionality
TEST_F(ComtradeParserTest, ClearData) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    ASSERT_TRUE(parser.isLoaded());
    
    parser.clear();
    
    EXPECT_FALSE(parser.isLoaded());
    EXPECT_EQ(parser.getTotalSamples(), 0);
}

// Test sample rate retrieval
TEST_F(ComtradeParserTest, GetSampleRate) {
    createTestCfgFile("test.cfg");
    createTestDatAsciiFile("test.dat", 5);
    
    ComtradeParser parser;
    ASSERT_TRUE(parser.load(testDir_ + "test.cfg"));
    
    // All samples should have rate 4800 Hz
    EXPECT_DOUBLE_EQ(parser.getSampleRate(0), 4800.0);
    EXPECT_DOUBLE_EQ(parser.getSampleRate(50), 4800.0);
    EXPECT_DOUBLE_EQ(parser.getSampleRate(99), 4800.0);
}
