#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <nlohmann/json.hpp>
#include "compat.hpp"  // Must include first for platform detection

#ifdef __APPLE__
#include "bpf_macos.hpp"
#endif

#ifdef _WIN32
#include "npcap_windows.hpp"
#endif

enum class DataSource {
    MANUAL,
    COMTRADE,
    CSV
};

struct SVConfig {
    std::string appId;
    std::string macDst;
    std::string macSrc;
    uint16_t vlanId;
    uint8_t vlanPrio;
    std::string svId;
    std::string dstAddress;
    double nominalFreq;
    uint32_t sampleRate;
    DataSource dataSource;
    std::string filePath; // for COMTRADE/CSV
};

struct Phasor {
    double magnitude;
    double angle;
};

class SVPublisherInstance {
public:
    SVPublisherInstance(const std::string& id, const SVConfig& config);
    ~SVPublisherInstance();

    // Delete copy constructor and copy assignment (instance owns resources)
    SVPublisherInstance(const SVPublisherInstance&) = delete;
    SVPublisherInstance& operator=(const SVPublisherInstance&) = delete;

    // Allow move operations (we manage this manually in the implementation)
    SVPublisherInstance(SVPublisherInstance&& other) noexcept;
    SVPublisherInstance& operator=(SVPublisherInstance&& other) noexcept;

    // Control
    void start();
    void stop();
    bool isRunning() const { return running_; }

    // Configuration
    const SVConfig& getConfig() const { return config_; }
    void setConfig(const SVConfig& config);

    // Phasor updates (for manual mode)
    void setPhasors(const std::vector<Phasor>& phasors);
    const std::vector<Phasor>& getPhasors() const { return phasors_; }

    // Harmonics (for manual mode)
    void setHarmonics(const nlohmann::json& harmonics);
    const nlohmann::json& getHarmonics() const { return harmonics_; }

    // Tick function
    void tick();

    // Serialization
    nlohmann::json toJson() const;

    const std::string& getId() const { return id_; }

private:
    std::string id_;
    SVConfig config_;
    bool running_;
    std::vector<Phasor> phasors_;
    nlohmann::json harmonics_;
    uint32_t sampleCounter_;
    
#ifdef __APPLE__
    vts::platform::BPFSocket* bpfSocket_;  // BPF socket for macOS
#endif
#ifdef _WIN32
    vts::platform::NpcapSocket* npcapSocket_;  // Npcap socket for Windows
#endif
    int rawSocket_;  // Linux raw socket fd, BPF fd on macOS, or Npcap handle on Windows

    void sendSVPacket();
    std::vector<int16_t> generateSamples();
    void initRawSocket();
    void closeRawSocket();
};
