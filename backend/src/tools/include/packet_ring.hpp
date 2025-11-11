#ifndef PACKET_RING_HPP
#define PACKET_RING_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

// Phase 8: High-performance packet I/O with TPACKET_V3 ring buffers (Linux-specific)
// Provides zero-copy packet capture and transmission using kernel ring buffers.

/**
 * PacketRing: TPACKET_V3 ring buffer for high-performance packet I/O
 * 
 * Features:
 * - Zero-copy packet capture/transmission
 * - Hardware timestamping support (best-effort)
 * - BPF filtering for protocol-specific capture
 * - Multi-reader fanout support
 * - Qdisc bypass for low-latency TX
 * 
 * Requires: Linux 3.2+ for TPACKET_V3, NET_RAW capability
 */
class PacketRing {
public:
    enum class RingType {
        RX_ONLY,    // Receive only (PACKET_RX_RING)
        TX_ONLY,    // Transmit only (PACKET_TX_RING)
        RX_TX       // Both RX and TX rings
    };

    /**
     * Constructor: Initialize packet ring (does not create socket)
     * interface_name: Network interface (e.g., "eth0")
     * ring_type: Type of ring(s) to create
     */
    PacketRing(const std::string& interface_name, RingType ring_type = RingType::RX_ONLY);
    
    /**
     * Destructor: Cleanup resources
     */
    ~PacketRing();

    // Disable copy (move semantics could be added later)
    PacketRing(const PacketRing&) = delete;
    PacketRing& operator=(const PacketRing&) = delete;

    /**
     * Initialize the packet ring:
     * 1. Create AF_PACKET socket
     * 2. Set up ring buffers (RX/TX)
     * 3. Configure promiscuous mode
     * 4. Set up BPF filters
     * 5. Configure timestamping
     * 
     * Returns: true on success, false on failure
     */
    bool initialize();

    /**
     * Add BPF filter for specific EtherType(s)
     * ethertypes: Vector of EtherTypes to capture (e.g., {0x88ba, 0x88b8})
     * include_vlan: Also capture VLAN-tagged packets (0x8100)
     * 
     * Call before initialize(). Returns: true on success
     */
    bool add_bpf_filter(const std::vector<uint16_t>& ethertypes, bool include_vlan = true);

    /**
     * Enable hardware timestamping (best-effort)
     * Falls back to software timestamping if HW not available.
     * 
     * Call before initialize(). Returns: true on success
     */
    bool enable_timestamping();

    /**
     * Enable PACKET_FANOUT for load-balancing across multiple sockets
     * fanout_id: Group ID for fanout (0-65535)
     * 
     * Call after initialize(). Returns: true on success
     */
    bool enable_fanout(uint16_t fanout_id);

    /**
     * Enable PACKET_QDISC_BYPASS for TX (bypass kernel qdisc layer)
     * 
     * Call after initialize(). Returns: true on success
     */
    bool enable_qdisc_bypass();

    /**
     * Get next available packet from RX ring (non-blocking poll)
     * 
     * Returns: Pointer to packet data, or nullptr if no packet available
     * size_out: Set to packet size in bytes
     * timestamp_ns_out: Set to packet timestamp in nanoseconds (if available)
     * 
     * Caller must call release_packet() when done with packet data.
     */
    const uint8_t* get_next_packet(size_t& size_out, uint64_t& timestamp_ns_out);

    /**
     * Release packet back to kernel (mark block as available)
     * Must be called after processing packet from get_next_packet().
     */
    void release_packet();

    /**
     * Send packet via TX ring (zero-copy if possible)
     * data: Packet data to send
     * size: Packet size in bytes
     * 
     * Returns: true on success, false on failure
     */
    bool send_packet(const uint8_t* data, size_t size);

    /**
     * Get socket file descriptor (for select/poll/epoll integration)
     */
    int get_fd() const { return socket_fd; }

    /**
     * Get statistics: packets received, packets dropped, etc.
     */
    struct Stats {
        uint64_t packets_received;
        uint64_t packets_dropped;
        uint64_t packets_sent;
        uint64_t errors;
    };
    Stats get_stats() const;

private:
    std::string interface_name;
    [[maybe_unused]] RingType ring_type;  // Reserved for future TX ring implementation
    int socket_fd;
    [[maybe_unused]] int if_index;  // Used in Linux-only promiscuous mode
    
    // Ring buffer configuration
    size_t block_size;
    size_t block_count;
    size_t frame_size;
    size_t frame_count;
    
    // Memory-mapped ring buffer
    [[maybe_unused]] void* ring_buffer;  // Reserved for future TX ring implementation
    [[maybe_unused]] size_t ring_buffer_size;  // Reserved for future TX ring implementation
    
    // Current block/frame indices
    [[maybe_unused]] size_t current_block_idx;  // Reserved for future TX ring implementation
    [[maybe_unused]] size_t current_frame_idx;  // Reserved for future TX ring implementation
    
    // BPF filter configuration
    std::vector<uint16_t> filter_ethertypes;
    bool filter_include_vlan;
    
    // Timestamping configuration
    bool timestamping_enabled;
    [[maybe_unused]] bool hw_timestamping_available;  // Reserved for future hardware timestamping
    
    // Statistics
    Stats stats;
    
    // Helper methods
    bool create_socket();
    bool setup_rx_ring();
    bool setup_tx_ring();
    bool bind_socket();
    bool set_promiscuous_mode(bool enable);
    bool apply_bpf_filter();
    bool configure_timestamping();
};

#endif // PACKET_RING_HPP
