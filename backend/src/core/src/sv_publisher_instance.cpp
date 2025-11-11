#include "sv_publisher_instance.hpp"
#include <cstring>
#include <cmath>
#include <stdexcept>
#include <iostream>

#ifdef __linux__
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#elif defined(__APPLE__)
#include <unistd.h>
#include "bpf_macos.hpp"  // Use our new BPF wrapper
#elif defined(_WIN32)
#include "npcap_windows.hpp"  // Use Npcap wrapper for Windows
#define htons(x) _byteswap_ushort(x)
#define htonl(x) _byteswap_ulong(x)
#endif

// SV protocol constants
constexpr uint16_t ETHERTYPE_8021Q = 0x8100;
constexpr uint16_t ETHERTYPE_SV = 0x88BA;
constexpr size_t MAX_SV_FRAME_SIZE = 1518;
constexpr int16_t SCALE_FACTOR = 3276; // ~10% of int16 max for ±10V

SVPublisherInstance::SVPublisherInstance(const std::string& id, const SVConfig& config)
    : id_(id)
    , config_(config)
    , running_(false)
    , sampleCounter_(0)
#ifdef __APPLE__
    , bpfSocket_(nullptr)
#endif
#ifdef _WIN32
    , npcapSocket_(nullptr)
#endif
    , rawSocket_(-1)
{
    // Initialize with zero phasors if manual mode
    if (config_.dataSource == DataSource::MANUAL) {
        phasors_.resize(8, {0.0, 0.0}); // Default 8 channels
    }
    
    harmonics_ = nlohmann::json::array();
    
    initRawSocket();
}

SVPublisherInstance::~SVPublisherInstance() {
    stop();
    closeRawSocket();
}

// Move constructor
SVPublisherInstance::SVPublisherInstance(SVPublisherInstance&& other) noexcept
    : id_(std::move(other.id_))
    , config_(std::move(other.config_))
    , running_(other.running_)
    , phasors_(std::move(other.phasors_))
    , harmonics_(std::move(other.harmonics_))
    , sampleCounter_(other.sampleCounter_)
#ifdef __APPLE__
    , bpfSocket_(other.bpfSocket_)
#endif
#ifdef _WIN32
    , npcapSocket_(other.npcapSocket_)
#endif
    , rawSocket_(other.rawSocket_)
{
    // Take ownership of resources
#ifdef __APPLE__
    other.bpfSocket_ = nullptr;
#endif
#ifdef _WIN32
    other.npcapSocket_ = nullptr;
#endif
    other.rawSocket_ = -1;
}

// Move assignment operator
SVPublisherInstance& SVPublisherInstance::operator=(SVPublisherInstance&& other) noexcept {
    if (this != &other) {
        // Clean up existing resources
        stop();
        closeRawSocket();
        
        // Move data
        id_ = std::move(other.id_);
        config_ = std::move(other.config_);
        running_ = other.running_;
        phasors_ = std::move(other.phasors_);
        harmonics_ = std::move(other.harmonics_);
        sampleCounter_ = other.sampleCounter_;
        
#ifdef __APPLE__
        bpfSocket_ = other.bpfSocket_;
        other.bpfSocket_ = nullptr;
#endif
#ifdef _WIN32
        npcapSocket_ = other.npcapSocket_;
        other.npcapSocket_ = nullptr;
#endif
        rawSocket_ = other.rawSocket_;
        other.rawSocket_ = -1;
    }
    return *this;
}

void SVPublisherInstance::initRawSocket() {
#ifdef __linux__
    // Create raw socket for sending Ethernet frames (Linux)
    rawSocket_ = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
    if (rawSocket_ < 0) {
        throw std::runtime_error("Failed to create raw socket: " + std::string(strerror(errno)));
    }
#elif defined(__APPLE__)
    // On macOS, use BPF (Berkeley Packet Filter) for raw packet access
    bpfSocket_ = new vts::platform::BPFSocket();
    
    // Get available network interfaces
    auto interfaces = vts::platform::getNetworkInterfaces();
    if (interfaces.empty()) {
        delete bpfSocket_;
        bpfSocket_ = nullptr;
        throw std::runtime_error("No network interfaces available");
    }
    
    // Try to open the first available interface (typically en0)
    std::string interface = interfaces[0];
    std::cout << "[SV Publisher] Using network interface: " << interface << std::endl;
    
    if (!bpfSocket_->open(interface)) {
        delete bpfSocket_;
        bpfSocket_ = nullptr;
        throw std::runtime_error("Failed to open BPF socket (requires sudo)");
    }
    
    // Set header complete mode for sending
    rawSocket_ = bpfSocket_->getFd();
    
    std::cout << "[SV Publisher] BPF socket initialized on " << interface << std::endl;
#elif defined(_WIN32)
    // On Windows, use Npcap for raw packet access
    npcapSocket_ = new vts::platform::NpcapSocket();
    
    // Get available network interfaces
    auto interfaces = vts::platform::getNetworkInterfacesWithNames();
    if (interfaces.empty()) {
        delete npcapSocket_;
        npcapSocket_ = nullptr;
        throw std::runtime_error("No network interfaces available (Is Npcap installed?)");
    }
    
    // Try to open the first available interface
    std::string interface = interfaces[0].first;
    std::string friendlyName = interfaces[0].second;
    std::cout << "[SV Publisher] Using network interface: " << friendlyName << std::endl;
    
    if (!npcapSocket_->open(interface)) {
        delete npcapSocket_;
        npcapSocket_ = nullptr;
        throw std::runtime_error("Failed to open Npcap socket (requires Npcap and may need Administrator)");
    }
    
    rawSocket_ = 0;  // Dummy value for Windows
    
    std::cout << "[SV Publisher] Npcap socket initialized on " << friendlyName << std::endl;
#else
    #error "Unsupported platform for raw sockets"
#endif
}

void SVPublisherInstance::closeRawSocket() {
#ifdef __APPLE__
    if (bpfSocket_ != nullptr) {
        delete bpfSocket_;
        bpfSocket_ = nullptr;
    }
#endif
#ifdef _WIN32
    if (npcapSocket_ != nullptr) {
        delete npcapSocket_;
        npcapSocket_ = nullptr;
    }
#endif
#ifdef __linux__
    if (rawSocket_ >= 0) {
        close(rawSocket_);
        rawSocket_ = -1;
    }
#else
    rawSocket_ = -1;
#endif
}

void SVPublisherInstance::start() {
    running_ = true;
    sampleCounter_ = 0;
}

void SVPublisherInstance::stop() {
    running_ = false;
}

void SVPublisherInstance::setConfig(const SVConfig& config) {
    config_ = config;
}

void SVPublisherInstance::setPhasors(const std::vector<Phasor>& phasors) {
    phasors_ = phasors;
}

void SVPublisherInstance::setHarmonics(const nlohmann::json& harmonics) {
    harmonics_ = harmonics;
}

std::vector<int16_t> SVPublisherInstance::generateSamples() {
    std::vector<int16_t> samples;
    
    if (config_.dataSource == DataSource::MANUAL) {
        // Generate samples from phasors using synthesis
        // For now, simple fundamental generation
        double t = static_cast<double>(sampleCounter_) / config_.sampleRate;
        
        for (const auto& phasor : phasors_) {
            // Convert phasor to instantaneous value
            // v(t) = √2 * V * sin(2πft + φ)
            double omega = 2.0 * M_PI * config_.nominalFreq;
            double phase = omega * t + (phasor.angle * M_PI / 180.0);
            double value = std::sqrt(2.0) * phasor.magnitude * std::sin(phase);
            
            // Scale to int16 range
            int16_t scaled = static_cast<int16_t>(value * SCALE_FACTOR);
            samples.push_back(scaled);
        }
    } else if (config_.dataSource == DataSource::COMTRADE) {
        // TODO: Read from COMTRADE file
        samples.resize(phasors_.size(), 0);
    } else if (config_.dataSource == DataSource::CSV) {
        // TODO: Read from CSV file
        samples.resize(phasors_.size(), 0);
    }
    
    return samples;
}

void SVPublisherInstance::sendSVPacket() {
    if (rawSocket_ < 0) {
        return;
    }
    
    // Generate samples for this tick
    std::vector<int16_t> samples = generateSamples();
    
    // Build Ethernet + VLAN + SV frame
    uint8_t frame[MAX_SV_FRAME_SIZE];
    size_t offset = 0;
    
    // Ethernet header
    // Destination MAC
    sscanf(config_.macDst.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &frame[0], &frame[1], &frame[2], &frame[3], &frame[4], &frame[5]);
    offset += 6;
    
    // Source MAC
    sscanf(config_.macSrc.c_str(), "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
           &frame[6], &frame[7], &frame[8], &frame[9], &frame[10], &frame[11]);
    offset += 6;
    
    // VLAN tag
    uint16_t vlanTag = htons(ETHERTYPE_8021Q);
    memcpy(&frame[offset], &vlanTag, 2);
    offset += 2;
    
    // VLAN TCI (Priority + VLAN ID)
    uint16_t tci = static_cast<uint16_t>((config_.vlanPrio << 13) | (config_.vlanId & 0x0FFF));
    tci = htons(tci);
    memcpy(&frame[offset], &tci, 2);
    offset += 2;
    
    // EtherType for SV
    uint16_t svType = htons(ETHERTYPE_SV);
    memcpy(&frame[offset], &svType, 2);
    offset += 2;
    
    // SV APDU (simplified - real implementation needs proper ASN.1 encoding)
    // APPID
    uint16_t appId = static_cast<uint16_t>(std::stoul(config_.appId, nullptr, 16));
    appId = htons(appId);
    memcpy(&frame[offset], &appId, 2);
    offset += 2;
    
    // Length (placeholder)
    uint16_t length = htons(static_cast<uint16_t>(samples.size() * 8 + 20));
    memcpy(&frame[offset], &length, 2);
    offset += 2;
    
    // Reserved fields
    frame[offset++] = 0x80;
    frame[offset++] = 0x00;
    
    // svID (simplified)
    uint8_t svIdLen = static_cast<uint8_t>(config_.svId.length());
    frame[offset++] = svIdLen;
    memcpy(&frame[offset], config_.svId.c_str(), svIdLen);
    offset += svIdLen;
    
    // smpCnt
    uint16_t smpCnt = htons(static_cast<uint16_t>(sampleCounter_ % config_.sampleRate));
    memcpy(&frame[offset], &smpCnt, 2);
    offset += 2;
    
    // confRev
    uint32_t confRev = htonl(1);
    memcpy(&frame[offset], &confRev, 4);
    offset += 4;
    
    // smpSynch
    frame[offset++] = 0x01; // Synced to external clock
    
    // Sample values (each as INT32Q with quality)
    for (int16_t sample : samples) {
        uint32_t value = htonl(static_cast<uint32_t>(static_cast<int32_t>(sample)));
        memcpy(&frame[offset], &value, 4);
        offset += 4;
        
        // Quality (0 = good)
        uint32_t quality = 0;
        memcpy(&frame[offset], &quality, 4);
        offset += 4;
    }
    
    // Send frame (platform-specific)
#ifdef __linux__
    struct sockaddr_ll sa;
    memset(&sa, 0, sizeof(sa));
    sa.sll_family = AF_PACKET;
    sa.sll_protocol = htons(ETH_P_ALL);
    sa.sll_ifindex = 0; // Use first available interface
    
    ssize_t sent = sendto(rawSocket_, frame, offset, 0, 
                          reinterpret_cast<struct sockaddr*>(&sa), sizeof(sa));
    
    if (sent < 0) {
        // Ignore send errors - they happen if no interface is available
    }
#elif defined(__APPLE__)
    // On macOS, use BPF write() to send raw Ethernet frame
    if (bpfSocket_ != nullptr && bpfSocket_->isOpen()) {
        ssize_t sent = bpfSocket_->write(frame, offset);
        
        if (sent < 0) {
            // Ignore send errors - they happen if interface is down or permissions issue
            // In production, log this error
        }
    }
#elif defined(_WIN32)
    // On Windows, use Npcap write() to send raw Ethernet frame
    if (npcapSocket_ != nullptr && npcapSocket_->isOpen()) {
        ssize_t sent = npcapSocket_->write(frame, offset);
        
        if (sent < 0) {
            // Ignore send errors - they happen if interface is down or permissions issue
            // In production, log this error
        }
    }
#endif
}

void SVPublisherInstance::tick() {
    if (!running_) {
        return;
    }
    
    sendSVPacket();
    sampleCounter_++;
}

nlohmann::json SVPublisherInstance::toJson() const {
    nlohmann::json j;
    j["id"] = id_;
    j["appId"] = config_.appId;
    j["macDst"] = config_.macDst;
    j["macSrc"] = config_.macSrc;
    j["vlanId"] = config_.vlanId;
    j["vlanPrio"] = config_.vlanPrio;
    j["svId"] = config_.svId;
    j["dstAddress"] = config_.dstAddress;
    j["nominalFreq"] = config_.nominalFreq;
    j["sampleRate"] = config_.sampleRate;
    j["running"] = running_;
    
    // Data source
    switch (config_.dataSource) {
        case DataSource::MANUAL:
            j["dataSource"] = "MANUAL";
            break;
        case DataSource::COMTRADE:
            j["dataSource"] = "COMTRADE";
            j["filePath"] = config_.filePath;
            break;
        case DataSource::CSV:
            j["dataSource"] = "CSV";
            j["filePath"] = config_.filePath;
            break;
    }
    
    // Phasors
    nlohmann::json phasorsJson = nlohmann::json::array();
    for (const auto& phasor : phasors_) {
        phasorsJson.push_back({
            {"magnitude", phasor.magnitude},
            {"angle", phasor.angle}
        });
    }
    j["phasors"] = phasorsJson;
    j["harmonics"] = harmonics_;
    
    return j;
}
