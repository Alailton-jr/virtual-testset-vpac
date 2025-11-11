#include "packet_ring.hpp"
#include <iostream>
#include <cstring>
#include <cerrno>

#ifdef __linux__
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/ethernet.h>
#ifdef __linux__
#include <linux/if_packet.h>
#endif
#include <linux/filter.h>
#ifdef __linux__
#include <linux/net_tstamp.h>
#endif
#ifdef __linux__
#include <linux/sockios.h>
#endif
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#endif

// TPACKET_V3 ring buffer defaults (tuned for IEC 61850 traffic)
constexpr size_t DEFAULT_BLOCK_SIZE = 4096 * 4;      // 16 KB blocks
constexpr size_t DEFAULT_BLOCK_COUNT = 256;          // 4 MB total ring
constexpr size_t DEFAULT_FRAME_SIZE = 2048;          // Max Ethernet frame
[[maybe_unused]] constexpr uint32_t DEFAULT_BLOCK_TIMEOUT_MS = 10;    // Poll timeout (reserved for future use)

PacketRing::PacketRing(const std::string& interface_name, RingType ring_type)
    : interface_name(interface_name)
    , ring_type(ring_type)
    , socket_fd(-1)
    , if_index(0)
    , block_size(DEFAULT_BLOCK_SIZE)
    , block_count(DEFAULT_BLOCK_COUNT)
    , frame_size(DEFAULT_FRAME_SIZE)
    , frame_count(0)
    , ring_buffer(nullptr)
    , ring_buffer_size(0)
    , current_block_idx(0)
    , current_frame_idx(0)
    , filter_include_vlan(true)
    , timestamping_enabled(false)
    , hw_timestamping_available(false)
    , stats({0, 0, 0, 0})
{
    frame_count = (block_size * block_count) / frame_size;
}

PacketRing::~PacketRing() {
#ifdef __linux__
    if (ring_buffer != nullptr && ring_buffer != MAP_FAILED) {
        munmap(ring_buffer, ring_buffer_size);
    }
    
    if (socket_fd >= 0) {
        set_promiscuous_mode(false);  // Disable promiscuous mode
        close(socket_fd);
    }
#endif
}

bool PacketRing::add_bpf_filter(const std::vector<uint16_t>& ethertypes, bool include_vlan) {
    filter_ethertypes = ethertypes;
    filter_include_vlan = include_vlan;
    return true;
}

bool PacketRing::enable_timestamping() {
    timestamping_enabled = true;
    return true;
}

bool PacketRing::initialize() {
#ifdef __linux__
    std::cout << "[PacketRing] Initializing " << interface_name << "..." << std::endl;
    
    if (!create_socket()) {
        return false;
    }
    
    if (!bind_socket()) {
        return false;
    }
    
    if (!set_promiscuous_mode(true)) {
        std::cerr << "[PacketRing] Warning: Failed to enable promiscuous mode" << std::endl;
    }
    
    if (!filter_ethertypes.empty() && !apply_bpf_filter()) {
        std::cerr << "[PacketRing] Warning: Failed to apply BPF filter" << std::endl;
    }
    
    if (timestamping_enabled && !configure_timestamping()) {
        std::cerr << "[PacketRing] Warning: Failed to configure timestamping" << std::endl;
    }
    
    if (ring_type == RingType::RX_ONLY || ring_type == RingType::RX_TX) {
        if (!setup_rx_ring()) {
            return false;
        }
    }
    
    if (ring_type == RingType::TX_ONLY || ring_type == RingType::RX_TX) {
        if (!setup_tx_ring()) {
            return false;
        }
    }
    
    std::cout << "[PacketRing] Initialized successfully" << std::endl;
    return true;
#else
    std::cerr << "[PacketRing] Error: TPACKET_V3 only supported on Linux" << std::endl;
    return false;
#endif
}

bool PacketRing::enable_fanout(uint16_t fanout_id) {
#ifdef __linux__
    if (socket_fd < 0) {
        std::cerr << "[PacketRing] Error: Socket not initialized" << std::endl;
        return false;
    }
    
    // PACKET_FANOUT_HASH for load balancing based on packet hash
    uint32_t fanout_arg = (fanout_id | (PACKET_FANOUT_HASH << 16));
    
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_FANOUT, &fanout_arg, sizeof(fanout_arg)) == -1) {
        std::cerr << "[PacketRing] Warning: PACKET_FANOUT failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "[PacketRing] PACKET_FANOUT enabled (ID=" << fanout_id << ")" << std::endl;
    return true;
#else
    (void)fanout_id;  // Unused on non-Linux platforms
    return false;
#endif
}

bool PacketRing::enable_qdisc_bypass() {
#ifdef __linux__
    if (socket_fd < 0) {
        std::cerr << "[PacketRing] Error: Socket not initialized" << std::endl;
        return false;
    }
    
    int bypass = 1;
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_QDISC_BYPASS, &bypass, sizeof(bypass)) == -1) {
        std::cerr << "[PacketRing] Warning: PACKET_QDISC_BYPASS failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "[PacketRing] PACKET_QDISC_BYPASS enabled" << std::endl;
    return true;
#else
    return false;
#endif
}

const uint8_t* PacketRing::get_next_packet(size_t& size_out, uint64_t& timestamp_ns_out) {
#ifdef __linux__
    // TODO: Implement TPACKET_V3 block iteration
    // This is a placeholder - full implementation requires:
    // 1. Check current block status (TP_STATUS_USER)
    // 2. Iterate frames in block
    // 3. Extract packet data and timestamp
    // 4. Return pointer to packet data
    
    size_out = 0;
    timestamp_ns_out = 0;
    return nullptr;
#else
    size_out = 0;
    timestamp_ns_out = 0;
    return nullptr;
#endif
}

void PacketRing::release_packet() {
#ifdef __linux__
    // TODO: Mark current block as released (TP_STATUS_KERNEL)
#endif
}

bool PacketRing::send_packet(const uint8_t* data, size_t size) {
#ifdef __linux__
    // TODO: Implement TX ring packet sending
    // This is a placeholder - full implementation requires:
    // 1. Find available TX frame
    // 2. Copy packet data to frame
    // 3. Mark frame as ready (TP_STATUS_SEND_REQUEST)
    // 4. Trigger send with sendto()
    
    return false;
#else
    (void)data;  // Unused on non-Linux platforms
    (void)size;  // Unused on non-Linux platforms
    return false;
#endif
}

PacketRing::Stats PacketRing::get_stats() const {
    return stats;
}

// Private helper methods

bool PacketRing::create_socket() {
#ifdef __linux__
    socket_fd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (socket_fd < 0) {
        std::cerr << "[PacketRing] Error: socket() failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    // Get interface index
    if_index = if_nametoindex(interface_name.c_str());
    if (if_index == 0) {
        std::cerr << "[PacketRing] Error: if_nametoindex(" << interface_name << ") failed: " 
                  << std::strerror(errno) << std::endl;
        close(socket_fd);
        socket_fd = -1;
        return false;
    }
    
    std::cout << "[PacketRing] Created socket (fd=" << socket_fd << ", if_index=" << if_index << ")" << std::endl;
    return true;
#else
    return false;
#endif
}

bool PacketRing::setup_rx_ring() {
#ifdef __linux__
    struct tpacket_req3 req;
    std::memset(&req, 0, sizeof(req));
    
    req.tp_block_size = block_size;
    req.tp_block_nr = block_count;
    req.tp_frame_size = frame_size;
    req.tp_frame_nr = frame_count;
    req.tp_retire_blk_tov = DEFAULT_BLOCK_TIMEOUT_MS;  // Block timeout in ms
    req.tp_feature_req_word = TP_FT_REQ_FILL_RXHASH;   // Request RX hash
    
    if (setsockopt(socket_fd, SOL_PACKET, PACKET_RX_RING, &req, sizeof(req)) == -1) {
        std::cerr << "[PacketRing] Error: PACKET_RX_RING failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    // Memory-map the ring buffer
    ring_buffer_size = req.tp_block_size * req.tp_block_nr;
    ring_buffer = mmap(nullptr, ring_buffer_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_LOCKED, socket_fd, 0);
    
    if (ring_buffer == MAP_FAILED) {
        std::cerr << "[PacketRing] Error: mmap() failed: " << std::strerror(errno) << std::endl;
        ring_buffer = nullptr;
        return false;
    }
    
    std::cout << "[PacketRing] RX ring configured: " 
              << block_count << " blocks × " << block_size << " bytes = " 
              << (ring_buffer_size / 1024 / 1024) << " MB" << std::endl;
    return true;
#else
    return false;
#endif
}

bool PacketRing::setup_tx_ring() {
#ifdef __linux__
    // TX ring setup similar to RX, but using PACKET_TX_RING
    // For now, return true (placeholder)
    std::cout << "[PacketRing] TX ring not yet implemented" << std::endl;
    return true;
#else
    return false;
#endif
}

bool PacketRing::bind_socket() {
#ifdef __linux__
    struct sockaddr_ll sll;
    std::memset(&sll, 0, sizeof(sll));
    
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ALL);
    sll.sll_ifindex = if_index;
    
    if (bind(socket_fd, reinterpret_cast<struct sockaddr*>(&sll), sizeof(sll)) == -1) {
        std::cerr << "[PacketRing] Error: bind() failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "[PacketRing] Socket bound to " << interface_name << std::endl;
    return true;
#else
    return false;
#endif
}

bool PacketRing::set_promiscuous_mode(bool enable) {
#ifdef __linux__
    struct packet_mreq mreq;
    std::memset(&mreq, 0, sizeof(mreq));
    
    mreq.mr_ifindex = if_index;
    mreq.mr_type = PACKET_MR_PROMISC;
    
    int action = enable ? PACKET_ADD_MEMBERSHIP : PACKET_DROP_MEMBERSHIP;
    
    if (setsockopt(socket_fd, SOL_PACKET, action, &mreq, sizeof(mreq)) == -1) {
        std::cerr << "[PacketRing] Warning: Promiscuous mode " << (enable ? "enable" : "disable") 
                  << " failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    if (enable) {
        std::cout << "[PacketRing] Promiscuous mode enabled" << std::endl;
    }
    return true;
#else
    (void)enable;  // Unused on non-Linux platforms
    return false;
#endif
}

bool PacketRing::apply_bpf_filter() {
#ifdef __linux__
    // Phase 8: BPF filter for SV (0x88ba) and GOOSE (0x88b8), including VLAN (0x8100)
    // 
    // BPF program structure:
    // - Load EtherType (12-13 bytes into Ethernet header)
    // - Check if it matches our filter list
    // - If VLAN (0x8100), check inner EtherType (16-17 bytes)
    // - Accept or reject packet
    
    std::vector<struct sock_filter> bpf_code;
    
    // Load EtherType at offset 12 (network byte order)
    bpf_code.push_back({0x28, 0, 0, 0x0000000c});  // ldh [12]
    
    // Check for each EtherType in filter list
    for (size_t i = 0; i < filter_ethertypes.size(); ++i) {
        uint16_t ethertype = filter_ethertypes[i];
        // jeq #ethertype, accept, next
        bpf_code.push_back({0x15, 0, static_cast<uint8_t>(filter_include_vlan ? 1 : 0), ethertype});
        if (!filter_include_vlan && i < filter_ethertypes.size() - 1) {
            bpf_code.back().jf = static_cast<uint8_t>(filter_ethertypes.size() - i);
        }
    }
    
    if (filter_include_vlan) {
        // Check for VLAN tag (0x8100)
        bpf_code.push_back({0x15, 0, 5, 0x8100});  // jeq #0x8100, check_inner, reject
        
        // Load inner EtherType at offset 16 (after VLAN tag)
        bpf_code.push_back({0x28, 0, 0, 0x00000010});  // ldh [16]
        
        // Check inner EtherType against filter list
        for (size_t i = 0; i < filter_ethertypes.size(); ++i) {
            uint16_t ethertype = filter_ethertypes[i];
            uint8_t jf = (i == filter_ethertypes.size() - 1) ? 1 : 0;
            bpf_code.push_back({0x15, 0, jf, ethertype});  // jeq #ethertype, accept, next/reject
        }
    }
    
    // Accept packet (return -1, unlimited bytes)
    bpf_code.push_back({0x06, 0, 0, 0xffffffff});  // ret #-1
    
    // Reject packet (return 0 bytes)
    bpf_code.push_back({0x06, 0, 0, 0x00000000});  // ret #0
    
    // Apply BPF program to socket
    struct sock_fprog bpf;
    bpf.len = bpf_code.size();
    bpf.filter = bpf_code.data();
    
    if (setsockopt(socket_fd, SOL_SOCKET, SO_ATTACH_FILTER, &bpf, sizeof(bpf)) == -1) {
        std::cerr << "[PacketRing] Warning: SO_ATTACH_FILTER failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "[PacketRing] BPF filter applied for EtherTypes: ";
    for (uint16_t et : filter_ethertypes) {
        std::cout << "0x" << std::hex << et << " ";
    }
    std::cout << std::dec << (filter_include_vlan ? "(+VLAN)" : "") << std::endl;
    return true;
#else
    return false;
#endif
}

bool PacketRing::configure_timestamping() {
#ifdef __linux__
    // Try hardware timestamping first (best-effort)
    struct ifreq ifr;
    struct hwtstamp_config hwts_config;
    
    std::memset(&ifr, 0, sizeof(ifr));
    std::memset(&hwts_config, 0, sizeof(hwts_config));
    
    strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    
    hwts_config.tx_type = HWTSTAMP_TX_OFF;
    hwts_config.rx_filter = HWTSTAMP_FILTER_ALL;
    
    ifr.ifr_data = reinterpret_cast<char*>(&hwts_config);
    
    if (ioctl(socket_fd, SIOCSHWTSTAMP, &ifr) == 0) {
        hw_timestamping_available = true;
        std::cout << "[PacketRing] Hardware timestamping enabled" << std::endl;
    } else {
        std::cerr << "[PacketRing] Warning: Hardware timestamping not available: " 
                  << std::strerror(errno) << std::endl;
    }
    
    // Enable software timestamping as fallback
    int ts_flags = SOF_TIMESTAMPING_RX_SOFTWARE | SOF_TIMESTAMPING_SOFTWARE;
    if (hw_timestamping_available) {
        ts_flags |= SOF_TIMESTAMPING_RX_HARDWARE | SOF_TIMESTAMPING_RAW_HARDWARE;
    }
    
    if (setsockopt(socket_fd, SOL_SOCKET, SO_TIMESTAMPING, &ts_flags, sizeof(ts_flags)) == -1) {
        std::cerr << "[PacketRing] Warning: SO_TIMESTAMPING failed: " << std::strerror(errno) << std::endl;
        return false;
    }
    
    std::cout << "[PacketRing] Software timestamping enabled" << std::endl;
    return true;
#else
    return false;
#endif
}
