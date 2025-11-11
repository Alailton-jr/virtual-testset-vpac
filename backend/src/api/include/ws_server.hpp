#ifndef WS_SERVER_HPP
#define WS_SERVER_HPP

#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <set>
#include <mutex>
#include <map>
#include <thread>
#include <atomic>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using websocketpp::connection_hdl;

// WebSocket server type
using server = websocketpp::server<websocketpp::config::asio>;

// Topic subscription types
enum class Topic {
    ANALYZER_PHASORS,      // Live phasor updates from analyzer
    ANALYZER_WAVEFORMS,    // Live waveform data
    ANALYZER_HARMONICS,    // Harmonics analysis results
    SEQUENCE_PROGRESS,     // Test sequence state updates
    GOOSE_EVENTS,          // GOOSE message events
    STREAM_STATUS          // SV stream status updates
};

// Connection with topic subscriptions
struct Connection {
    connection_hdl hdl;
    std::set<Topic> topics;
};

class WSServer {
public:
    WSServer(int port);
    ~WSServer();

    // Server control
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }

    // Broadcasting
    void broadcast(Topic topic, const json& data);
    void broadcastToAll(const json& data);

    // Connection stats
    size_t getConnectionCount() const;
    size_t getSubscriberCount(Topic topic) const;

private:
    // WebSocket event handlers
    void onOpen(connection_hdl hdl);
    void onClose(connection_hdl hdl);
    void onMessage(connection_hdl hdl, server::message_ptr msg);

    // Message handling
    void handleSubscribe(connection_hdl hdl, const json& payload);
    void handleUnsubscribe(connection_hdl hdl, const json& payload);
    void handlePing(connection_hdl hdl);

    // Utility
    Topic stringToTopic(const std::string& topicStr) const;
    std::string topicToString(Topic topic) const;

    int port_;
    std::atomic<bool> running_;
    server server_;
    std::thread serverThread_;

    // Connection management
    std::map<connection_hdl, Connection, std::owner_less<connection_hdl>> connections_;
    mutable std::mutex mutex_;
};

#endif // WS_SERVER_HPP
