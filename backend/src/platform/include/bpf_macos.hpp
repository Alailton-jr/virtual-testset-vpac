#ifndef PLATFORM_BPF_MACOS_HPP
#define PLATFORM_BPF_MACOS_HPP

#ifdef VTS_PLATFORM_MAC

#include <string>
#include <vector>
#include <cstdint>

namespace vts {
namespace platform {

/**
 * @brief macOS BPF (Berkeley Packet Filter) wrapper for packet capture and transmission
 * 
 * BPF on macOS provides access to the data link layer for packet capture and injection.
 * Requires root/sudo privileges to open BPF devices.
 * 
 * Usage:
 *   BPFSocket bpf;
 *   if (bpf.open("en0")) {
 *       std::vector<uint8_t> packet = bpf.read();
 *       bpf.write(packet.data(), packet.size());
 *   }
 */
class BPFSocket {
public:
    BPFSocket();
    ~BPFSocket();

    /**
     * @brief Open a BPF device and bind to network interface
     * @param interface Network interface name (e.g., "en0", "en1")
     * @return true on success, false on failure
     */
    bool open(const std::string& interface);

    /**
     * @brief Close the BPF device
     */
    void close();

    /**
     * @brief Check if BPF device is open
     * @return true if open, false otherwise
     */
    bool isOpen() const { return fd_ >= 0; }

    /**
     * @brief Read a packet from the network interface
     * @return Packet data (empty vector on error or timeout)
     */
    std::vector<uint8_t> read();

    /**
     * @brief Write a packet to the network interface
     * @param data Packet data
     * @param length Packet length
     * @return Number of bytes written, -1 on error
     */
    ssize_t write(const uint8_t* data, size_t length);

    /**
     * @brief Set read timeout
     * @param timeout_ms Timeout in milliseconds (0 = non-blocking)
     */
    void setTimeout(uint32_t timeout_ms);

    /**
     * @brief Enable/disable promiscuous mode
     * @param enable true to enable, false to disable
     * @return true on success, false on failure
     */
    bool setPromiscuous(bool enable);

    /**
     * @brief Set BPF filter (e.g., to capture only specific protocols)
     * @param filter_expression BPF filter string (e.g., "ip", "ether proto 0x88ba")
     * @return true on success, false on failure
     */
    bool setFilter(const std::string& filter_expression);

    /**
     * @brief Get BPF buffer size
     * @return Buffer size in bytes
     */
    size_t getBufferSize() const { return buffer_size_; }

    /**
     * @brief Get interface name
     * @return Interface name
     */
    const std::string& getInterface() const { return interface_; }

    /**
     * @brief Get interface MAC address
     * @return MAC address as 6-byte vector (empty on error)
     */
    std::vector<uint8_t> getMacAddress() const;

    /**
     * @brief Get file descriptor (for select/poll)
     * @return BPF file descriptor, -1 if not open
     */
    int getFd() const { return fd_; }

private:
    int fd_;                        // BPF device file descriptor
    std::string interface_;         // Network interface name
    size_t buffer_size_;            // BPF buffer size
    std::vector<uint8_t> buffer_;   // Read buffer
    size_t buffer_offset_;          // Current offset in buffer
    size_t buffer_len_;             // Valid data length in buffer

    /**
     * @brief Find and open an available BPF device (/dev/bpf0, /dev/bpf1, ...)
     * @return File descriptor on success, -1 on failure
     */
    int openBPFDevice();

    /**
     * @brief Configure BPF device
     * @return true on success, false on failure
     */
    bool configureBPF();
};

/**
 * @brief Network interface information
 */
struct NetworkInterfaceInfo {
    std::string name;
    std::string macAddress;
    std::string ipAddress;
    bool isActive;
};

/**
 * @brief Get list of available network interfaces on macOS
 * @return Vector of interface names
 */
std::vector<std::string> getNetworkInterfaces();

/**
 * @brief Get detailed information about network interfaces
 * @return Vector of NetworkInterfaceInfo structs
 */
std::vector<NetworkInterfaceInfo> getNetworkInterfacesDetailed();

/**
 * @brief Check if running with root/sudo privileges
 * @return true if root, false otherwise
 */
bool isRoot();

} // namespace platform
} // namespace vts

#endif // VTS_PLATFORM_MAC

#endif // PLATFORM_BPF_MACOS_HPP
