#ifndef RAW_SOCKET_STUB_HPP
#define RAW_SOCKET_STUB_HPP

// ============================================================================
// RawSocket Stub for macOS (Phase 11)
// ============================================================================
// This header provides a safe stub implementation of RawSocket for platforms
// that don't support AF_PACKET (macOS, BSD, Windows).
//
// All methods are no-ops that log INFO/WARNING messages and return safe values.
// This allows the codebase to compile and run in --no-net mode without
// network I/O functionality.
//
// Usage:
//   Selected automatically via platform guards in CMakeLists.txt
//   On macOS: Always use this stub
//   On Linux: Use real raw_socket.hpp
// ============================================================================

#include <memory>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sstream>
#include <iomanip>

// Stub structures to match Linux API
struct sockaddr_ll {
    uint16_t sll_family;
    uint16_t sll_protocol;
    int32_t  sll_ifindex;
    uint16_t sll_hatype;
    uint8_t  sll_pkttype;
    uint8_t  sll_halen;
    uint8_t  sll_addr[8];
};

struct msghdr_stub {
    void*       msg_name;
    uint32_t    msg_namelen;
    void*       msg_iov;
    uint32_t    msg_iovlen;
    void*       msg_control;
    uint32_t    msg_controllen;
    int         msg_flags;
};

struct iovec_stub {
    void*   iov_base;
    size_t  iov_len;
};

class RawSocket {
public:
    int32_t socket_id;
    struct sockaddr_ll bind_addr;
    int32_t if_index;
    msghdr_stub msg_hdr;
    struct iovec_stub iov;

public:
    RawSocket() {
        std::cout << "[NET] INFO: RawSocket stub initialized (macOS - no network operations)" << std::endl;
        
        // Initialize with safe stub values
        socket_id = -1;
        if_index = 1; // Fake interface index
        
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sll_family = 0; // AF_PACKET not available
        bind_addr.sll_ifindex = if_index;
        
        config_MsgHdr();
    }

    ~RawSocket() {
        std::cout << "[NET] INFO: RawSocket stub destroyed" << std::endl;
        // Nothing to close - socket was never opened
    }

private:
    void create_socket(uint8_t pushFrames2driver, uint8_t txRing, uint8_t timestamping, uint8_t fanout, uint8_t rxTimeout) {
        std::cout << "[NET] INFO: RawSocket::create_socket() stub - no operation performed" << std::endl;
        std::cout << "[NET]       Options requested: qdisc_bypass=" << static_cast<int>(pushFrames2driver)
                  << " tx_ring=" << static_cast<int>(txRing)
                  << " timestamping=" << static_cast<int>(timestamping)
                  << " fanout=" << static_cast<int>(fanout)
                  << " rx_timeout=" << static_cast<int>(rxTimeout) << std::endl;
        
        // Set stub values
        socket_id = -1; // Invalid socket
        if_index = 1;   // Fake interface index
        
        // Don't throw - allow no-net mode to continue
    }

    void config_MsgHdr() {
        std::cout << "[NET] INFO: RawSocket::config_MsgHdr() stub - no operation performed" << std::endl;
        
        memset(&msg_hdr, 0, sizeof(msg_hdr));
        memset(&iov, 0, sizeof(iov));
        
        // Stub configuration
        msg_hdr.msg_name = &bind_addr;
        msg_hdr.msg_namelen = sizeof(bind_addr);
        iov.iov_base = nullptr;
        iov.iov_len = 0;
        msg_hdr.msg_iov = &iov;
        msg_hdr.msg_iovlen = 1;
        msg_hdr.msg_control = nullptr;
        msg_hdr.msg_controllen = 0;
        msg_hdr.msg_flags = 0;
    }
};

// Stub implementation of GetMACAddress for macOS
inline const std::string GetMACAddress(const char* interface) {
    std::cout << "[NET] INFO: GetMACAddress(" << interface << ") stub - returning dummy MAC" << std::endl;
    
    // Return a well-known dummy MAC address
    return "00:00:00:00:00:00";
}

#endif // RAW_SOCKET_STUB_HPP
