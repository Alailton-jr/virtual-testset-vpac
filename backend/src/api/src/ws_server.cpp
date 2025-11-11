#include "ws_server.hpp"
#include <iostream>
#include <algorithm>

using websocketpp::lib::placeholders::_1;
using websocketpp::lib::placeholders::_2;
using websocketpp::lib::bind;

WSServer::WSServer(int port)
    : port_(port), running_(false) {
    
    // Initialize Asio
    server_.init_asio();
    
    // Set logging
    server_.set_access_channels(websocketpp::log::alevel::none);
    server_.set_error_channels(websocketpp::log::elevel::none);
    
    // Register handlers
    server_.set_open_handler(bind(&WSServer::onOpen, this, _1));
    server_.set_close_handler(bind(&WSServer::onClose, this, _1));
    server_.set_message_handler(bind(&WSServer::onMessage, this, _1, _2));
    
    // Allow connection reuse
    server_.set_reuse_addr(true);
}

WSServer::~WSServer() {
    stop();
}

void WSServer::start() {
    if (running_.load()) {
        std::cerr << "WebSocket server already running" << std::endl;
        return;
    }
    
    running_.store(true);
    
    serverThread_ = std::thread([this]() {
        try {
            std::cout << "WebSocket server starting on port " << port_ << std::endl;
            
            server_.listen(port_);
            server_.start_accept();
            server_.run();
            
            std::cout << "WebSocket server stopped" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "WebSocket server error: " << e.what() << std::endl;
            running_.store(false);
        }
    });
    
    std::cout << "WebSocket server started on port " << port_ << std::endl;
}

void WSServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    
    // Close all connections
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& pair : connections_) {
            try {
                server_.close(pair.first, websocketpp::close::status::going_away, "Server shutting down");
            } catch (...) {
                // Ignore errors during shutdown
            }
        }
        connections_.clear();
    }
    
    // Stop the server
    server_.stop();
    
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    
    std::cout << "WebSocket server stopped" << std::endl;
}

void WSServer::onOpen(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Connection conn;
    conn.hdl = hdl;
    connections_[hdl] = conn;
    
    std::cout << "WebSocket connection opened (total: " << connections_.size() << ")" << std::endl;
    
    // Send welcome message
    try {
        json welcome = {
            {"type", "welcome"},
            {"message", "Connected to Virtual TestSet WebSocket"},
            {"availableTopics", {
                "analyzer/phasors",
                "analyzer/waveforms",
                "analyzer/harmonics",
                "sequence/progress",
                "goose/events",
                "stream/status"
            }}
        };
        server_.send(hdl, welcome.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        std::cerr << "Error sending welcome message: " << e.what() << std::endl;
    }
}

void WSServer::onClose(connection_hdl hdl) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = connections_.find(hdl);
    if (it != connections_.end()) {
        connections_.erase(it);
    }
    
    std::cout << "WebSocket connection closed (total: " << connections_.size() << ")" << std::endl;
}

void WSServer::onMessage(connection_hdl hdl, server::message_ptr msg) {
    try {
        json payload = json::parse(msg->get_payload());
        std::string msgType = payload.value("type", "");
        
        if (msgType == "subscribe") {
            handleSubscribe(hdl, payload);
        } else if (msgType == "unsubscribe") {
            handleUnsubscribe(hdl, payload);
        } else if (msgType == "ping") {
            handlePing(hdl);
        } else {
            // Unknown message type
            json error = {
                {"type", "error"},
                {"message", "Unknown message type: " + msgType}
            };
            server_.send(hdl, error.dump(), websocketpp::frame::opcode::text);
        }
    } catch (const json::exception& e) {
        json error = {
            {"type", "error"},
            {"message", std::string("Invalid JSON: ") + e.what()}
        };
        try {
            server_.send(hdl, error.dump(), websocketpp::frame::opcode::text);
        } catch (...) {
            // Ignore send errors
        }
    } catch (const std::exception& e) {
        std::cerr << "Error handling message: " << e.what() << std::endl;
    }
}

void WSServer::handleSubscribe(connection_hdl hdl, const json& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = connections_.find(hdl);
    if (it == connections_.end()) {
        return;
    }
    
    std::string topicStr = payload.value("topic", "");
    Topic topic = stringToTopic(topicStr);
    
    it->second.topics.insert(topic);
    
    std::cout << "Client subscribed to topic: " << topicStr << std::endl;
    
    // Send confirmation
    json response = {
        {"type", "subscribed"},
        {"topic", topicStr}
    };
    
    try {
        server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        std::cerr << "Error sending subscription confirmation: " << e.what() << std::endl;
    }
}

void WSServer::handleUnsubscribe(connection_hdl hdl, const json& payload) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = connections_.find(hdl);
    if (it == connections_.end()) {
        return;
    }
    
    std::string topicStr = payload.value("topic", "");
    Topic topic = stringToTopic(topicStr);
    
    it->second.topics.erase(topic);
    
    std::cout << "Client unsubscribed from topic: " << topicStr << std::endl;
    
    // Send confirmation
    json response = {
        {"type", "unsubscribed"},
        {"topic", topicStr}
    };
    
    try {
        server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        std::cerr << "Error sending unsubscription confirmation: " << e.what() << std::endl;
    }
}

void WSServer::handlePing(connection_hdl hdl) {
    json response = {
        {"type", "pong"},
        {"timestamp", std::time(nullptr)}
    };
    
    try {
        server_.send(hdl, response.dump(), websocketpp::frame::opcode::text);
    } catch (const std::exception& e) {
        std::cerr << "Error sending pong: " << e.what() << std::endl;
    }
}

void WSServer::broadcast(Topic topic, const json& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json message = {
        {"type", "data"},
        {"topic", topicToString(topic)},
        {"data", data},
        {"timestamp", std::time(nullptr)}
    };
    
    std::string payload = message.dump();
    
    for (auto& pair : connections_) {
        if (pair.second.topics.count(topic) > 0) {
            try {
                server_.send(pair.first, payload, websocketpp::frame::opcode::text);
            } catch (const std::exception& e) {
                std::cerr << "Error broadcasting to client: " << e.what() << std::endl;
            }
        }
    }
}

void WSServer::broadcastToAll(const json& data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    json message = {
        {"type", "broadcast"},
        {"data", data},
        {"timestamp", std::time(nullptr)}
    };
    
    std::string payload = message.dump();
    
    for (auto& pair : connections_) {
        try {
            server_.send(pair.first, payload, websocketpp::frame::opcode::text);
        } catch (const std::exception& e) {
            std::cerr << "Error broadcasting to client: " << e.what() << std::endl;
        }
    }
}

size_t WSServer::getConnectionCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connections_.size();
}

size_t WSServer::getSubscriberCount(Topic topic) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    size_t count = 0;
    for (const auto& pair : connections_) {
        if (pair.second.topics.count(topic) > 0) {
            count++;
        }
    }
    return count;
}

Topic WSServer::stringToTopic(const std::string& topicStr) const {
    if (topicStr == "analyzer/phasors") return Topic::ANALYZER_PHASORS;
    if (topicStr == "analyzer/waveforms") return Topic::ANALYZER_WAVEFORMS;
    if (topicStr == "analyzer/harmonics") return Topic::ANALYZER_HARMONICS;
    if (topicStr == "sequence/progress") return Topic::SEQUENCE_PROGRESS;
    if (topicStr == "goose/events") return Topic::GOOSE_EVENTS;
    if (topicStr == "stream/status") return Topic::STREAM_STATUS;
    
    // Default to analyzer phasors
    return Topic::ANALYZER_PHASORS;
}

std::string WSServer::topicToString(Topic topic) const {
    switch (topic) {
        case Topic::ANALYZER_PHASORS: return "analyzer/phasors";
        case Topic::ANALYZER_WAVEFORMS: return "analyzer/waveforms";
        case Topic::ANALYZER_HARMONICS: return "analyzer/harmonics";
        case Topic::SEQUENCE_PROGRESS: return "sequence/progress";
        case Topic::GOOSE_EVENTS: return "goose/events";
        case Topic::STREAM_STATUS: return "stream/status";
        default: return "unknown";
    }
}
