#include "compat.hpp"  // Must include first for platform detection

#ifdef VTS_PLATFORM_MAC

#include "bpf_macos.hpp"
#include <iostream>
#include <cstring>
#include <algorithm>
#include <map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/bpf.h>
#include <ifaddrs.h>
#include <net/if_dl.h>
#include <arpa/inet.h>

namespace vts {
namespace platform {

BPFSocket::BPFSocket() 
    : fd_(-1), 
      buffer_size_(0), 
      buffer_offset_(0), 
      buffer_len_(0) {
}

BPFSocket::~BPFSocket() {
    close();
}

int BPFSocket::openBPFDevice() {
    // Try to open /dev/bpf0 through /dev/bpf255
    for (int i = 0; i < 256; i++) {
        std::string bpf_device = "/dev/bpf" + std::to_string(i);
        int fd = ::open(bpf_device.c_str(), O_RDWR);
        if (fd >= 0) {
            std::cout << "[BPF] Opened " << bpf_device << std::endl;
            return fd;
        }
    }
    
    std::cerr << "[BPF] Error: Could not open any BPF device (/dev/bpf0-255)" << std::endl;
    std::cerr << "[BPF] Make sure you have root privileges (run with sudo)" << std::endl;
    return -1;
}

bool BPFSocket::configureBPF() {
    // Set immediate mode (packets delivered immediately, not buffered)
    unsigned int immediate = 1;
    if (ioctl(fd_, BIOCIMMEDIATE, &immediate) < 0) {
        std::cerr << "[BPF] Error: BIOCIMMEDIATE failed: " << std::strerror(errno) << std::endl;
        return false;
    }

    // Get buffer size
    unsigned int buf_len;
    if (ioctl(fd_, BIOCGBLEN, &buf_len) < 0) {
        std::cerr << "[BPF] Error: BIOCGBLEN failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    buffer_size_ = buf_len;
    buffer_.resize(buffer_size_);
    
    std::cout << "[BPF] Buffer size: " << buffer_size_ << " bytes" << std::endl;

    // Enable "see sent" packets (optional - see your own transmitted packets)
    unsigned int see_sent = 1;
    if (ioctl(fd_, BIOCSSEESENT, &see_sent) < 0) {
        std::cerr << "[BPF] Warning: BIOCSSEESENT failed: " << std::strerror(errno) << std::endl;
    }

    // Set header complete mode (we provide full Ethernet frame)
    unsigned int hdr_complete = 1;
    if (ioctl(fd_, BIOCSHDRCMPLT, &hdr_complete) < 0) {
        std::cerr << "[BPF] Error: BIOCSHDRCMPLT failed: " << std::strerror(errno) << std::endl;
        return false;
    }

    return true;
}

bool BPFSocket::open(const std::string& interface) {
    if (isOpen()) {
        std::cerr << "[BPF] Error: BPF device already open" << std::endl;
        return false;
    }

    // Check for root privileges
    if (!isRoot()) {
        std::cerr << "[BPF] Error: Root privileges required. Run with sudo." << std::endl;
        return false;
    }

    // Open BPF device
    fd_ = openBPFDevice();
    if (fd_ < 0) {
        return false;
    }

    // Bind to network interface
    struct ifreq ifr;
    std::strncpy(ifr.ifr_name, interface.c_str(), IFNAMSIZ);
    if (ioctl(fd_, BIOCSETIF, &ifr) < 0) {
        std::cerr << "[BPF] Error: BIOCSETIF failed for " << interface 
                  << ": " << std::strerror(errno) << std::endl;
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    interface_ = interface;
    std::cout << "[BPF] Bound to interface: " << interface_ << std::endl;

    // Configure BPF
    if (!configureBPF()) {
        ::close(fd_);
        fd_ = -1;
        return false;
    }

    std::cout << "[BPF] Successfully opened and configured BPF on " << interface_ << std::endl;
    return true;
}

void BPFSocket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
        interface_.clear();
        buffer_offset_ = 0;
        buffer_len_ = 0;
        std::cout << "[BPF] Closed BPF device" << std::endl;
    }
}

std::vector<uint8_t> BPFSocket::read() {
    if (!isOpen()) {
        return {};
    }

    // If we have buffered data, read from buffer
    if (buffer_offset_ < buffer_len_) {
        struct bpf_hdr* bpf_hdr = (struct bpf_hdr*)(buffer_.data() + buffer_offset_);
        
        // Extract packet data (skip BPF header)
        uint8_t* packet_data = buffer_.data() + buffer_offset_ + bpf_hdr->bh_hdrlen;
        size_t packet_len = bpf_hdr->bh_caplen;
        
        std::vector<uint8_t> packet(packet_data, packet_data + packet_len);
        
        // Move to next packet in buffer
        buffer_offset_ += BPF_WORDALIGN(bpf_hdr->bh_hdrlen + bpf_hdr->bh_caplen);
        
        return packet;
    }

    // Read new data from BPF device
    ssize_t n = ::read(fd_, buffer_.data(), buffer_size_);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // Timeout or non-blocking mode
            return {};
        }
        std::cerr << "[BPF] Error: read() failed: " << std::strerror(errno) << std::endl;
        return {};
    }

    if (n == 0) {
        // No data available
        return {};
    }

    // Process first packet from new buffer
    buffer_len_ = n;
    buffer_offset_ = 0;
    
    struct bpf_hdr* bpf_hdr = (struct bpf_hdr*)buffer_.data();
    uint8_t* packet_data = buffer_.data() + bpf_hdr->bh_hdrlen;
    size_t packet_len = bpf_hdr->bh_caplen;
    
    std::vector<uint8_t> packet(packet_data, packet_data + packet_len);
    
    buffer_offset_ += BPF_WORDALIGN(bpf_hdr->bh_hdrlen + bpf_hdr->bh_caplen);
    
    return packet;
}

ssize_t BPFSocket::write(const uint8_t* data, size_t length) {
    if (!isOpen()) {
        std::cerr << "[BPF] Error: Cannot write, BPF device not open" << std::endl;
        return -1;
    }

    if (length < 14) {
        std::cerr << "[BPF] Error: Packet too short (must be at least 14 bytes for Ethernet)" << std::endl;
        return -1;
    }

    // Write raw Ethernet frame
    ssize_t n = ::write(fd_, data, length);
    if (n < 0) {
        std::cerr << "[BPF] Error: write() failed: " << std::strerror(errno) << std::endl;
        return -1;
    }

    return n;
}

void BPFSocket::setTimeout(uint32_t timeout_ms) {
    if (!isOpen()) {
        return;
    }

    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;

    if (ioctl(fd_, BIOCSRTIMEOUT, &tv) < 0) {
        std::cerr << "[BPF] Error: BIOCSRTIMEOUT failed: " << std::strerror(errno) << std::endl;
    }
}

bool BPFSocket::setPromiscuous(bool enable) {
    if (!isOpen()) {
        return false;
    }

    unsigned int promisc = enable ? 1 : 0;
    if (ioctl(fd_, BIOCPROMISC, &promisc) < 0) {
        std::cerr << "[BPF] Error: BIOCPROMISC failed: " << std::strerror(errno) << std::endl;
        return false;
    }

    std::cout << "[BPF] Promiscuous mode " << (enable ? "enabled" : "disabled") << std::endl;
    return true;
}

bool BPFSocket::setFilter(const std::string& filter_expression) {
    if (!isOpen()) {
        return false;
    }

    // TODO: Implement BPF filter compilation
    // This requires libpcap or manual BPF instruction generation
    std::cerr << "[BPF] Warning: Filter not implemented yet: " << filter_expression << std::endl;
    return false;
}

std::vector<uint8_t> BPFSocket::getMacAddress() const {
    if (interface_.empty()) {
        return {};
    }

    struct ifaddrs* ifap;
    if (getifaddrs(&ifap) != 0) {
        return {};
    }

    std::vector<uint8_t> mac;
    for (struct ifaddrs* ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr->sa_family == AF_LINK && 
            std::string(ifa->ifa_name) == interface_) {
            struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifa->ifa_addr;
            uint8_t* mac_addr = (uint8_t*)LLADDR(sdl);
            mac.assign(mac_addr, mac_addr + 6);
            break;
        }
    }

    freeifaddrs(ifap);
    return mac;
}

std::vector<std::string> getNetworkInterfaces() {
    std::vector<std::string> interfaces;
    
    struct ifaddrs* ifap;
    if (getifaddrs(&ifap) != 0) {
        std::cerr << "[BPF] Error: getifaddrs() failed: " << std::strerror(errno) << std::endl;
        return interfaces;
    }

    for (struct ifaddrs* ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK) {
            std::string name(ifa->ifa_name);
            // Filter out loopback, special interfaces, and inactive interfaces
            // Check if interface is UP and RUNNING
            if (name != "lo0" && 
                name.find("utun") != 0 && 
                name.find("awdl") != 0 &&
                name.find("llw") != 0 &&
                name.find("gif") != 0 &&
                name.find("stf") != 0 &&
                name.find("bridge") != 0 &&
                (ifa->ifa_flags & IFF_UP) &&
                (ifa->ifa_flags & IFF_RUNNING)) {
                // Only add if not already in list (avoid duplicates)
                if (std::find(interfaces.begin(), interfaces.end(), name) == interfaces.end()) {
                    interfaces.push_back(name);
                }
            }
        }
    }

    freeifaddrs(ifap);
    return interfaces;
}

std::vector<NetworkInterfaceInfo> getNetworkInterfacesDetailed() {
    std::vector<NetworkInterfaceInfo> interfaces;
    
    struct ifaddrs* ifap;
    if (getifaddrs(&ifap) != 0) {
        std::cerr << "[BPF] Error: getifaddrs() failed: " << std::strerror(errno) << std::endl;
        return interfaces;
    }

    // First pass: collect all interface names and basic info
    std::map<std::string, NetworkInterfaceInfo> ifaceMap;
    
    for (struct ifaddrs* ifa = ifap; ifa != nullptr; ifa = ifa->ifa_next) {
        // Safety check: skip if name is null
        if (!ifa->ifa_name) {
            continue;
        }
        
        std::string name(ifa->ifa_name);
        
        // Skip loopback and virtual interfaces
        if (name == "lo0" || 
            name.find("utun") == 0 || 
            name.find("awdl") == 0 ||
            name.find("llw") == 0 ||
            name.find("gif") == 0 ||
            name.find("stf") == 0 ||
            name.find("bridge") == 0) {
            continue;
        }
        
        // Initialize entry if not exists
        if (ifaceMap.find(name) == ifaceMap.end()) {
            NetworkInterfaceInfo info;
            info.name = name;
            info.macAddress = "";
            info.ipAddress = "";
            info.isActive = false;
            ifaceMap[name] = info;
        }
        
        // Check if interface is active (UP and RUNNING)
        if ((ifa->ifa_flags & IFF_UP) && (ifa->ifa_flags & IFF_RUNNING)) {
            ifaceMap[name].isActive = true;
        }
        
        // Get MAC address from AF_LINK
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_LINK) {
            struct sockaddr_dl* sdl = (struct sockaddr_dl*)ifa->ifa_addr;
            if (sdl->sdl_alen == 6) {
                unsigned char* ptr = (unsigned char*)LLADDR(sdl);
                char mac[18];
                snprintf(mac, sizeof(mac), "%02x:%02x:%02x:%02x:%02x:%02x",
                        ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5]);
                ifaceMap[name].macAddress = mac;
            }
        }
        
        // Get IPv4 address
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            struct sockaddr_in* sin = (struct sockaddr_in*)ifa->ifa_addr;
            char ip[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof(ip))) {
                ifaceMap[name].ipAddress = ip;
            }
        }
    }

    freeifaddrs(ifap);
    
    // Convert map to vector
    for (const auto& pair : ifaceMap) {
        interfaces.push_back(pair.second);
    }
    
    return interfaces;
}

bool isRoot() {
    return geteuid() == 0;
}

} // namespace platform
} // namespace vts

#endif // VTS_PLATFORM_MAC
