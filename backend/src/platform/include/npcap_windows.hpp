#ifndef NPCAP_WINDOWS_HPP
#define NPCAP_WINDOWS_HPP

#ifdef VTS_PLATFORM_WINDOWS

#include <cstdint>
#include <string>
#include <vector>

// Forward declarations to avoid including WinPcap/Npcap headers here
typedef struct pcap pcap_t;
typedef struct pcap_pkthdr pcap_pkthdr;

namespace vts {
namespace platform {

/**
 * @brief Windows network packet capture and transmission using Npcap/WinPcap
 * 
 * Npcap is the Windows port of libpcap, providing raw packet access on Windows.
 * It's the successor to WinPcap and is required for Virtual TestSet network operations.
 * 
 * Installation: https://npcap.com/ (includes WinPcap API compatibility)
 * 
 * Features:
 * - Raw Ethernet frame transmission
 * - Packet capture with filtering
 * - Interface enumeration
 * - Non-blocking I/O
 * 
 * Limitations:
 * - Requires Npcap driver installation (admin installer)
 * - May require running as Administrator for some interfaces
 * - Performance lower than Linux AF_PACKET or macOS BPF
 */
class NpcapSocket {
public:
    NpcapSocket();
    ~NpcapSocket();

    // Disable copy, allow move
    NpcapSocket(const NpcapSocket&) = delete;
    NpcapSocket& operator=(const NpcapSocket&) = delete;
    NpcapSocket(NpcapSocket&& other) noexcept;
    NpcapSocket& operator=(NpcapSocket&& other) noexcept;

    /**
     * @brief Open Npcap device and bind to network interface
     * @param interface Interface name (e.g., "\\Device\\NPF_{GUID}" or friendly name)
     * @return true on success, false on failure
     */
    bool open(const std::string& interface);

    /**
     * @brief Close Npcap device
     */
    void close();

    /**
     * @brief Read one packet from network (non-blocking)
     * @return Packet data (empty if no packet available)
     */
    std::vector<uint8_t> read();

    /**
     * @brief Write raw Ethernet frame to network
     * @param data Packet data (must include full Ethernet header)
     * @param length Packet length in bytes
     * @return Number of bytes sent, or -1 on error
     */
    ssize_t write(const uint8_t* data, size_t length);

    /**
     * @brief Set read timeout
     * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
     */
    void setTimeout(uint32_t timeout_ms);

    /**
     * @brief Set promiscuous mode
     * @param enable true to enable, false to disable
     * @return true on success
     */
    bool setPromiscuous(bool enable);

    /**
     * @brief Set BPF filter for packet capture
     * @param filter_expression BPF filter string (e.g., "ether proto 0x88ba")
     * @return true on success
     */
    bool setFilter(const std::string& filter_expression);

    /**
     * @brief Get MAC address of the interface
     * @return MAC address (6 bytes), empty on error
     */
    std::vector<uint8_t> getMacAddress() const;

    /**
     * @brief Get interface name
     * @return Interface name
     */
    const std::string& getInterface() const { return interface_; }

    /**
     * @brief Check if socket is open
     * @return true if open
     */
    bool isOpen() const { return pcap_handle_ != nullptr; }

private:
    pcap_t* pcap_handle_;           // Npcap handle
    std::string interface_;          // Interface name
    uint32_t timeout_ms_;            // Read timeout
    bool promiscuous_;               // Promiscuous mode enabled

    /**
     * @brief Configure Npcap parameters
     * @return true on success
     */
    bool configureNpcap();
};

/**
 * @brief Get list of available network interfaces
 * @return Vector of interface names (Npcap device names)
 */
std::vector<std::string> getNetworkInterfaces();

/**
 * @brief Get friendly names for network interfaces
 * @return Vector of pairs: {device_name, friendly_name}
 */
std::vector<std::pair<std::string, std::string>> getNetworkInterfacesWithNames();

/**
 * @brief Check if running with Administrator privileges
 * @return true if running as Admin
 */
bool isAdmin();

/**
 * @brief Check if Npcap is installed
 * @return true if Npcap/WinPcap is available
 */
bool isNpcapInstalled();

} // namespace platform
} // namespace vts

#endif // VTS_PLATFORM_WINDOWS
#endif // NPCAP_WINDOWS_HPP
