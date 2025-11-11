#ifdef __linux__
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <linux/sockios.h>
#include <linux/if_packet.h>
#include <linux/net_tstamp.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <iomanip>
#include <linux/if_ether.h>
#endif
#ifndef RAW_SOCKET_HPP
#define RAW_SOCKET_HPP

#include <cstdint>
#include <cstring>
#include <cerrno>
#include <iostream>
#include <sys/types.h>
#include "general_definition.hpp"

#ifdef __linux__
class RawSocket{
public:
    int32_t socket_id;
    struct sockaddr_ll bind_addr;
    int32_t if_index;
    msghdr msg_hdr;
    struct iovec iov;
public:
    RawSocket(){
        create_socket(0, 0, 0, 0, 0);
        config_MsgHdr();
    }
    ~RawSocket(){
        close(socket_id);
    }
private:
    void create_socket(uint8_t pushFrames2driver, uint8_t txRing, uint8_t timestamping, uint8_t fanout, uint8_t rxTimeout){
        socket_id = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        if(socket_id < 0){
            throw std::runtime_error("Failed to create socket: " + std::string(strerror(errno)));
        }
        std::string if_name = getInterfaceName();
        if_index = if_nametoindex(if_name.c_str());
        if (if_index == 0) {
            close(socket_id);
            throw std::runtime_error("Failed to get interface index for " + if_name + ": " + std::string(strerror(errno)));
        }
        memset(&bind_addr, 0, sizeof(bind_addr));
        bind_addr.sll_family   = AF_PACKET;
        bind_addr.sll_protocol = htons(ETH_P_ALL);
        bind_addr.sll_ifindex  = if_index;
        if (pushFrames2driver){
            static const int32_t sock_qdisc_bypass = 1;
            if (setsockopt(socket_id, SOL_PACKET, PACKET_QDISC_BYPASS, &sock_qdisc_bypass, sizeof(sock_qdisc_bypass)) == -1) {
                std::cerr << "Failed to set PACKET_QDISC_BYPASS: " << strerror(errno) << std::endl;
            }
        }
        if (txRing){
            static const int32_t sock_tx_ring = 1;
            if (setsockopt(socket_id, SOL_PACKET, PACKET_TX_RING, &sock_tx_ring, sizeof(sock_tx_ring)) == -1) {
                std::cerr << "Failed to set PACKET_TX_RING: " << strerror(errno) << std::endl;
            }
        }
        if (timestamping){
            static const int32_t sock_timestamp = SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
            if (setsockopt(socket_id, SOL_SOCKET, SO_TIMESTAMPING, &sock_timestamp, sizeof(sock_timestamp)) == -1) {
                std::cerr << "Failed to set SO_TIMESTAMPING: " << strerror(errno) << std::endl;
            }
        }
        if (fanout){
            static const int32_t sock_fanout = PACKET_FANOUT_HASH;
            if (setsockopt(socket_id, SOL_PACKET, PACKET_FANOUT, &sock_fanout, sizeof(sock_fanout)) == -1) {
                std::cerr << "Failed to set PACKET_FANOUT: " << strerror(errno) << std::endl;
            }
        }
        if (rxTimeout){
            struct timeval timeout;
            timeout.tv_sec  = 4;
            timeout.tv_usec = 0;
            if (setsockopt(socket_id, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) == -1) {
                std::cerr << "Failed to set SO_RCVTIMEO: " << strerror(errno) << std::endl;
            }
        }
    }
    void config_MsgHdr(){
        memset(&msg_hdr, 0, sizeof(msg_hdr));
        memset(&iov, 0, sizeof(iov));
        msg_hdr.msg_name = &bind_addr;
        msg_hdr.msg_namelen = sizeof(bind_addr);
        iov.iov_base = nullptr;
        iov.iov_len = 0;
        msg_hdr.msg_iov = &iov;
        msg_hdr.msg_iovlen = 1;
        msg_hdr.msg_control = NULL;
        msg_hdr.msg_controllen = 0;
    }
};
#else
class RawSocket{
public:
    int32_t socket_id;
    int32_t if_index;
public:
    RawSocket(){
        socket_id = -1;
        if_index = -1;
        std::cerr << "[RawSocket] Raw sockets are not supported on this platform." << std::endl;
    }
    ~RawSocket() {}
private:
    void create_socket(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t) {
        std::cerr << "[RawSocket] create_socket() called on unsupported platform." << std::endl;
    }

    void config_MsgHdr() {
        // No-op on non-Linux
    }
};
#endif


inline const std::string GetMACAddress([[maybe_unused]] const char* interface) {
#ifdef __linux__
    struct ifaddrs *ifaddr, *ifa;
    unsigned char *mac;
    std::ostringstream macAddressStream;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return "";
    }
    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_name == std::string(interface) && ifa->ifa_addr->sa_family == AF_PACKET) {
            mac = reinterpret_cast<unsigned char*>(ifa->ifa_addr->sa_data);
            macAddressStream << std::hex << std::setfill('0');
            for (int i = 0; i < 6; ++i) {
                macAddressStream << std::setw(2) << static_cast<int>(mac[i]);
                if (i < 5) macAddressStream << ":";
            }
            break;
        }
    }
    freeifaddrs(ifaddr);
    return macAddressStream.str();
#else
    return "";
#endif
}


#endif // RAW_SOCKET_HPP