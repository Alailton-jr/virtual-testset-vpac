#include "sv_publisher_manager.hpp"
#include <random>
#include <sstream>
#include <iomanip>
#include <stdexcept>

SVPublisherManager::SVPublisherManager() {
}

SVPublisherManager::~SVPublisherManager() {
    stopAll();
}

std::string SVPublisherManager::generateId() const {
    // Generate a simple UUID-like ID
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    
    for (int i = 0; i < 8; i++) {
        ss << std::setw(1) << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 4; i++) {
        ss << std::setw(1) << dis(gen);
    }
    ss << "-4"; // Version 4 UUID
    for (int i = 0; i < 3; i++) {
        ss << std::setw(1) << dis(gen);
    }
    ss << "-";
    ss << std::setw(1) << ((dis(gen) & 0x3) | 0x8); // Variant bits
    for (int i = 0; i < 3; i++) {
        ss << std::setw(1) << dis(gen);
    }
    ss << "-";
    for (int i = 0; i < 12; i++) {
        ss << std::setw(1) << dis(gen);
    }
    
    return ss.str();
}

SVConfig SVPublisherManager::parseConfig(const nlohmann::json& json) const {
    SVConfig config;
    
    config.appId = json.value("appId", "0x4000");
    config.macDst = json.value("macDst", "01:0C:CD:04:00:00");
    config.macSrc = json.value("macSrc", "AA:BB:CC:DD:EE:01");
    config.vlanId = static_cast<uint16_t>(json.value("vlanId", 100));
    config.vlanPrio = static_cast<uint8_t>(json.value("vlanPrio", 4));
    config.svId = json.value("svId", "IED1MU01");
    config.dstAddress = json.value("dstAddress", "");
    config.nominalFreq = json.value("nominalFreq", 60.0);
    config.sampleRate = static_cast<uint32_t>(json.value("sampleRate", 4800));
    
    // Parse data source
    std::string dataSourceStr = json.value("dataSource", "MANUAL");
    if (dataSourceStr == "MANUAL") {
        config.dataSource = DataSource::MANUAL;
    } else if (dataSourceStr == "COMTRADE") {
        config.dataSource = DataSource::COMTRADE;
        config.filePath = json.value("filePath", "");
    } else if (dataSourceStr == "CSV") {
        config.dataSource = DataSource::CSV;
        config.filePath = json.value("filePath", "");
    } else {
        throw std::invalid_argument("Invalid dataSource: " + dataSourceStr);
    }
    
    return config;
}

std::string SVPublisherManager::createStream(const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    SVConfig svConfig = parseConfig(config);
    std::string id = generateId();
    
    auto instance = std::make_shared<SVPublisherInstance>(id, svConfig);
    streams_[id] = instance;
    
    return id;
}

void SVPublisherManager::updateStream(const std::string& streamId, const nlohmann::json& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + streamId);
    }
    
    SVConfig svConfig = parseConfig(config);
    it->second->setConfig(svConfig);
}

void SVPublisherManager::deleteStream(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + streamId);
    }
    
    it->second->stop();
    streams_.erase(it);
}

nlohmann::json SVPublisherManager::listStreams() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json result = nlohmann::json::array();
    
    for (const auto& pair : streams_) {
        result.push_back(pair.second->toJson());
    }
    
    return result;
}

nlohmann::json SVPublisherManager::getStream(const std::string& streamId) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + streamId);
    }
    
    return it->second->toJson();
}

void SVPublisherManager::startStream(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + streamId);
    }
    
    it->second->start();
}

void SVPublisherManager::stopStream(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + streamId);
    }
    
    it->second->stop();
}

void SVPublisherManager::startAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : streams_) {
        pair.second->start();
    }
}

void SVPublisherManager::stopAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : streams_) {
        pair.second->stop();
    }
}

void SVPublisherManager::updatePhasors(const std::string& streamId, const nlohmann::json& phasorData) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + streamId);
    }
    
    // Parse phasor data
    std::vector<Phasor> phasors;
    if (phasorData.contains("phasors") && phasorData["phasors"].is_array()) {
        for (const auto& p : phasorData["phasors"]) {
            Phasor phasor;
            phasor.magnitude = p.value("magnitude", 0.0);
            phasor.angle = p.value("angle", 0.0);
            phasors.push_back(phasor);
        }
    }
    
    it->second->setPhasors(phasors);
}

void SVPublisherManager::updateHarmonics(const std::string& streamId, const nlohmann::json& harmonicsData) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        throw std::runtime_error("Stream not found: " + streamId);
    }
    
    it->second->setHarmonics(harmonicsData);
}

void SVPublisherManager::tickAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (auto& pair : streams_) {
        pair.second->tick();
    }
}

void SVPublisherManager::updateStreamPhasors(const std::string& streamId, double freq,
                                              const std::map<std::string, std::pair<double, double>>& channels) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        // Stream not found - log warning but don't throw
        return;
    }
    
    // Build JSON for updatePhasors
    nlohmann::json phasorData;
    phasorData["freq"] = freq;
    
    nlohmann::json channelsJson = nlohmann::json::object();
    for (const auto& [channelId, phasor] : channels) {
        channelsJson[channelId] = {
            {"mag", phasor.first},
            {"angleDeg", phasor.second}
        };
    }
    phasorData["channels"] = channelsJson;
    
    // Call the existing updatePhasors method which unlocks properly
    mutex_.unlock();
    updatePhasors(streamId, phasorData);
    mutex_.lock();
}

std::shared_ptr<SVPublisherInstance> SVPublisherManager::getInstance(const std::string& streamId) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = streams_.find(streamId);
    if (it == streams_.end()) {
        return nullptr;
    }
    
    return it->second;
}
