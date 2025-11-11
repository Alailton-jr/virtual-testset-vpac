#pragma once
#include <string>
#include <map>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include "sv_publisher_instance.hpp"

class SVPublisherManager {
public:
    SVPublisherManager();
    ~SVPublisherManager();

    // CRUD operations
    std::string createStream(const nlohmann::json& config);
    void updateStream(const std::string& streamId, const nlohmann::json& config);
    void deleteStream(const std::string& streamId);
    nlohmann::json listStreams() const;
    nlohmann::json getStream(const std::string& streamId) const;

    // Control
    void startStream(const std::string& streamId);
    void stopStream(const std::string& streamId);
    void startAll();
    void stopAll();

    // Updates
    void updatePhasors(const std::string& streamId, const nlohmann::json& phasorData);
    void updateHarmonics(const std::string& streamId, const nlohmann::json& harmonicsData);
    
    // Sequence engine integration
    void updateStreamPhasors(const std::string& streamId, double freq, 
                            const std::map<std::string, std::pair<double, double>>& channels);

    // High-resolution tick
    void tickAll();

    // Getters
    std::shared_ptr<SVPublisherInstance> getInstance(const std::string& streamId);

private:
    std::map<std::string, std::shared_ptr<SVPublisherInstance>> streams_;
    mutable std::mutex mutex_;

    std::string generateId() const;
    SVConfig parseConfig(const nlohmann::json& json) const;
};
