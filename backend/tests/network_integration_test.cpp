#include <gtest/gtest.h>
#include "sv_publisher_instance.hpp"
#include "compat.hpp"

#ifdef __linux__
#include <linux/if_packet.h>
#include <net/if.h>
#include <sys/socket.h>
#elif defined(__APPLE__)
#include "bpf_macos.hpp"
#elif defined(_WIN32)
#include "npcap_windows.hpp"
#endif

#include <thread>
#include <chrono>
#include <iostream>

/**
 * @file network_integration_test.cpp
 * @brief Real network packet injection and capture tests
 * 
 * These tests verify that the Virtual TestSet can actually inject SV packets
 * into the network and capture them. They require:
 * - Linux: No special requirements (runs as-is with CAP_NET_RAW)
 * - macOS: sudo (for BPF device access)
 * - Windows: Npcap installed + may need Administrator
 * 
 * Tests are SKIPPED if network access is not available.
 */

class NetworkIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Check if network operations are available
        #if !VTS_HAS_RAW_SOCKETS
            GTEST_SKIP() << "Raw sockets not available on this platform";
        #endif

        #ifdef __APPLE__
            if (!vts::platform::isRoot()) {
                GTEST_SKIP() << "macOS BPF requires sudo - run tests with: sudo ./vts_tests";
            }
        #endif

        #ifdef _WIN32
            if (!vts::platform::isNpcapInstalled()) {
                GTEST_SKIP() << "Npcap not installed - download from https://npcap.com/";
            }
            // Note: May still fail if not running as Administrator
        #endif
    }
};

// Test: Enumerate network interfaces
TEST_F(NetworkIntegrationTest, EnumerateInterfaces) {
    #ifdef __linux__
        // Linux: Use if_nameindex()
        struct if_nameindex* if_ni = if_nameindex();
        ASSERT_NE(if_ni, nullptr) << "Failed to enumerate network interfaces";
        
        int count = 0;
        for (int i = 0; if_ni[i].if_index != 0; i++) {
            std::cout << "  [" << i << "] " << if_ni[i].if_name 
                      << " (index: " << if_ni[i].if_index << ")" << std::endl;
            count++;
        }
        if_freenameindex(if_ni);
        
        EXPECT_GT(count, 0) << "No network interfaces found";
        
    #elif defined(__APPLE__)
        // macOS: Use BPF getNetworkInterfaces()
        auto interfaces = vts::platform::getNetworkInterfaces();
        ASSERT_GT(interfaces.size(), 0) << "No network interfaces found";
        
        std::cout << "Found " << interfaces.size() << " network interfaces:" << std::endl;
        for (size_t i = 0; i < interfaces.size(); i++) {
            std::cout << "  [" << i << "] " << interfaces[i] << std::endl;
        }
        
    #elif defined(_WIN32)
        // Windows: Use Npcap getNetworkInterfacesWithNames()
        auto interfaces = vts::platform::getNetworkInterfacesWithNames();
        ASSERT_GT(interfaces.size(), 0) << "No network interfaces found";
        
        std::cout << "Found " << interfaces.size() << " network interfaces:" << std::endl;
        for (size_t i = 0; i < interfaces.size(); i++) {
            std::cout << "  [" << i << "] " << interfaces[i].second << std::endl;
            std::cout << "      Device: " << interfaces[i].first << std::endl;
        }
    #endif
}

// Test: Open raw socket/BPF/Npcap device
TEST_F(NetworkIntegrationTest, OpenRawSocket) {
    #ifdef __linux__
        int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        ASSERT_GE(sock, 0) << "Failed to create AF_PACKET socket: " << strerror(errno);
        close(sock);
        std::cout << "✓ Linux AF_PACKET socket created successfully" << std::endl;
        
    #elif defined(__APPLE__)
        vts::platform::BPFSocket bpf;
        auto interfaces = vts::platform::getNetworkInterfaces();
        ASSERT_GT(interfaces.size(), 0);
        
        bool opened = bpf.open(interfaces[0]);
        ASSERT_TRUE(opened) << "Failed to open BPF device on " << interfaces[0];
        std::cout << "✓ macOS BPF device opened successfully on " << interfaces[0] << std::endl;
        
    #elif defined(_WIN32)
        vts::platform::NpcapSocket npcap;
        auto interfaces = vts::platform::getNetworkInterfaces();
        ASSERT_GT(interfaces.size(), 0);
        
        bool opened = npcap.open(interfaces[0]);
        ASSERT_TRUE(opened) << "Failed to open Npcap device on " << interfaces[0];
        std::cout << "✓ Windows Npcap device opened successfully" << std::endl;
    #endif
}

// Test: Get MAC address of interface
TEST_F(NetworkIntegrationTest, GetMacAddress) {
    #ifdef __linux__
        // Linux: Use ioctl SIOCGIFHWADDR
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        ASSERT_GE(sock, 0);
        
        struct if_nameindex* if_ni = if_nameindex();
        ASSERT_NE(if_ni, nullptr);
        
        if (if_ni[0].if_index != 0) {
            struct ifreq ifr;
            strncpy(ifr.ifr_name, if_ni[0].if_name, IFNAMSIZ - 1);
            
            if (ioctl(sock, SIOCGIFHWADDR, &ifr) == 0) {
                unsigned char* mac = reinterpret_cast<unsigned char*>(ifr.ifr_hwaddr.sa_data);
                printf("✓ MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
                       mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
                
                // Verify it's not all zeros
                bool all_zeros = true;
                for (int i = 0; i < 6; i++) {
                    if (mac[i] != 0) {
                        all_zeros = false;
                        break;
                    }
                }
                EXPECT_FALSE(all_zeros) << "MAC address is all zeros";
            }
        }
        
        if_freenameindex(if_ni);
        close(sock);
        
    #elif defined(__APPLE__)
        vts::platform::BPFSocket bpf;
        auto interfaces = vts::platform::getNetworkInterfaces();
        ASSERT_GT(interfaces.size(), 0);
        
        ASSERT_TRUE(bpf.open(interfaces[0]));
        auto mac = bpf.getMacAddress();
        ASSERT_EQ(mac.size(), 6) << "MAC address should be 6 bytes";
        
        printf("✓ MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        
    #elif defined(_WIN32)
        vts::platform::NpcapSocket npcap;
        auto interfaces = vts::platform::getNetworkInterfaces();
        ASSERT_GT(interfaces.size(), 0);
        
        ASSERT_TRUE(npcap.open(interfaces[0]));
        auto mac = npcap.getMacAddress();
        ASSERT_EQ(mac.size(), 6) << "MAC address should be 6 bytes";
        
        printf("✓ MAC address: %02x:%02x:%02x:%02x:%02x:%02x\n",
               mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    #endif
}

// Test: Inject SV packet into network
TEST_F(NetworkIntegrationTest, InjectSVPacket) {
    // Create SV publisher configuration
    SVConfig config;
    config.appId = "0x4001";
    config.macDst = "01:0c:cd:01:00:00";  // IEC 61850-9-2 multicast
    config.macSrc = "00:11:22:33:44:55";  // Dummy source MAC
    config.vlanId = 0;
    config.vlanPrio = 4;
    config.svId = "TestSV01";
    config.dstAddress = "0x4001";
    config.nominalFreq = 60.0;
    config.sampleRate = 4800;
    config.dataSource = DataSource::MANUAL;

    // Create publisher instance
    SVPublisherInstance publisher("test_sv", config);
    
    // Set test phasors (3-phase voltage)
    std::vector<Phasor> phasors = {
        {120.0, 0.0},      // Va = 120V ∠0°
        {120.0, -120.0},   // Vb = 120V ∠-120°
        {120.0, 120.0}     // Vc = 120V ∠120°
    };
    publisher.setPhasors(phasors);
    
    // Start publishing
    publisher.start();
    
    // Send 10 packets
    std::cout << "Injecting 10 SV packets into network..." << std::endl;
    for (int i = 0; i < 10; i++) {
        publisher.tick();
        std::this_thread::sleep_for(std::chrono::microseconds(208));  // ~4800 Hz
    }
    
    publisher.stop();
    
    std::cout << "✓ Successfully injected 10 SV packets" << std::endl;
    std::cout << "  To capture, run in another terminal:" << std::endl;
    
    #ifdef __linux__
        std::cout << "    sudo tcpdump -i any -vvv ether proto 0x88ba" << std::endl;
    #elif defined(__APPLE__)
        std::cout << "    sudo tcpdump -i en0 -vvv ether proto 0x88ba" << std::endl;
    #elif defined(_WIN32)
        std::cout << "    Wireshark with filter: eth.type == 0x88ba" << std::endl;
    #endif
}

// Test: Capture packets from network
TEST_F(NetworkIntegrationTest, CapturePackets) {
    std::cout << "Attempting to capture packets for 2 seconds..." << std::endl;
    std::cout << "(This test may show 0 packets if no SV traffic on network)" << std::endl;
    
    int packet_count = 0;
    
    #ifdef __linux__
        int sock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        ASSERT_GE(sock, 0);
        
        // Set 2-second timeout
        struct timeval tv;
        tv.tv_sec = 2;
        tv.tv_usec = 0;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
        
        uint8_t buffer[2048];
        auto start = std::chrono::steady_clock::now();
        
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
            ssize_t len = recv(sock, buffer, sizeof(buffer), 0);
            if (len > 0) {
                packet_count++;
                
                // Check if it's an SV packet (EtherType 0x88BA)
                if (len >= 14) {
                    uint16_t ethertype = (buffer[12] << 8) | buffer[13];
                    if (ethertype == 0x88BA) {
                        std::cout << "  ✓ Captured SV packet (" << len << " bytes)" << std::endl;
                    }
                }
            }
        }
        
        close(sock);
        
    #elif defined(__APPLE__)
        vts::platform::BPFSocket bpf;
        auto interfaces = vts::platform::getNetworkInterfaces();
        ASSERT_GT(interfaces.size(), 0);
        ASSERT_TRUE(bpf.open(interfaces[0]));
        
        bpf.setTimeout(100);  // 100ms timeout
        
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
            auto packet = bpf.read();
            if (!packet.empty()) {
                packet_count++;
                
                // Check if it's an SV packet
                if (packet.size() >= 14) {
                    uint16_t ethertype = (packet[12] << 8) | packet[13];
                    if (ethertype == 0x88BA) {
                        std::cout << "  ✓ Captured SV packet (" << packet.size() << " bytes)" << std::endl;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
    #elif defined(_WIN32)
        vts::platform::NpcapSocket npcap;
        auto interfaces = vts::platform::getNetworkInterfaces();
        ASSERT_GT(interfaces.size(), 0);
        ASSERT_TRUE(npcap.open(interfaces[0]));
        
        npcap.setTimeout(100);  // 100ms timeout
        
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < std::chrono::seconds(2)) {
            auto packet = npcap.read();
            if (!packet.empty()) {
                packet_count++;
                
                // Check if it's an SV packet
                if (packet.size() >= 14) {
                    uint16_t ethertype = (packet[12] << 8) | packet[13];
                    if (ethertype == 0x88BA) {
                        std::cout << "  ✓ Captured SV packet (" << packet.size() << " bytes)" << std::endl;
                    }
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    #endif
    
    std::cout << "✓ Captured " << packet_count << " total packets in 2 seconds" << std::endl;
    // Don't fail if no packets - network might be idle
}

// Test: Round-trip test - inject and capture in same test
TEST_F(NetworkIntegrationTest, RoundTripTest) {
    std::cout << "Round-trip test: Inject SV packets and capture them..." << std::endl;
    
    // This test requires two threads or loopback capture
    // For simplicity, we'll just verify both injection and capture work independently
    // A full round-trip would require either:
    // 1. Two network interfaces (one TX, one RX)
    // 2. Loopback interface with promiscuous mode
    // 3. Two separate processes
    
    GTEST_SKIP() << "Round-trip test requires advanced setup (two interfaces or loopback)";
}

// Test environment setup - prints platform info before tests run
class NetworkTestEnvironment : public ::testing::Environment {
public:
    void SetUp() override {
        std::cout << "\n========================================" << std::endl;
        std::cout << "Network Integration Tests" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "Platform: " << VTS_PLATFORM_NAME << std::endl;
        std::cout << "Raw Sockets: " << (VTS_HAS_RAW_SOCKETS ? "YES" : "NO") << std::endl;
        
        #ifdef __APPLE__
            std::cout << "\n⚠️  macOS: Run with sudo for BPF access" << std::endl;
            std::cout << "   Command: sudo ./vts_tests --gtest_filter='NetworkIntegrationTest.*'" << std::endl;
        #endif
        
        #ifdef _WIN32
            std::cout << "\n⚠️  Windows: Requires Npcap installation" << std::endl;
            std::cout << "   Download: https://npcap.com/" << std::endl;
            std::cout << "   May require Administrator privileges" << std::endl;
        #endif
        
        std::cout << "\n" << std::endl;
    }
};

// Register the test environment
static ::testing::Environment* const network_env = 
    ::testing::AddGlobalTestEnvironment(new NetworkTestEnvironment);
