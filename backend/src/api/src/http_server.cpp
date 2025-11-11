#include "http_server.hpp"
#include "sv_publisher_manager.hpp"
#include "sequence_engine.hpp"
#include "analyzer_engine.hpp"
#include "ws_server.hpp"
#include "impedance_calculator.hpp"
#include "ramping_tester.hpp"
#include "distance_tester.hpp"
#include "overcurrent_tester.hpp"
#include "differential_tester.hpp"
#include "global_flags.hpp"
#include "compat.hpp"
#ifdef VTS_PLATFORM_MAC
#include "bpf_macos.hpp"
#endif
#include <iostream>
#include <fstream>
#include <sstream>
#include <ctime>

// Using declarations for tester types to avoid namespace clutter
using vts::testers::RampVariable;
using vts::testers::RampConfig;
using vts::testers::RampResult;
using vts::testers::PhasorState;
using vts::testers::SourceImpedance;
using vts::testers::FaultType;
using vts::testers::DistancePoint;
using vts::testers::DistanceResult;
using vts::testers::DistanceTestConfig;
using vts::testers::OCSettings;
using vts::testers::OCPoint;
using vts::testers::OCResult;
using vts::testers::OCTestConfig;
using vts::testers::OCCurve;
using vts::testers::DifferentialPoint;
using vts::testers::DifferentialResult;
using vts::testers::DifferentialTestConfig;

HTTPServer::HTTPServer(int port)
    : port_(port), running_(false), wsServer_(nullptr) {
    server_ = std::make_unique<httplib::Server>();
    setupRoutes();
}

HTTPServer::~HTTPServer() {
    stop();
}

void HTTPServer::setupRoutes() {
    // CORS headers for development
    server_->set_post_routing_handler([](const httplib::Request& /*req*/, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
    });
    
    // OPTIONS handler for CORS preflight
    server_->Options(R"(.*)", [](const httplib::Request&, httplib::Response& res) {
        res.set_header("Access-Control-Allow-Origin", "*");
        res.set_header("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, OPTIONS");
        res.set_header("Access-Control-Allow-Headers", "Content-Type, Authorization");
        res.status = 204;
    });
    
    // Health endpoint
    server_->Get("/api/v1/health", [this](const httplib::Request& req, httplib::Response& res) {
        handleHealth(req, res);
    });
    
    // Stream management endpoints (Module 13)
    server_->Get("/api/v1/streams", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetStreams(req, res);
    });
    
    server_->Post("/api/v1/streams", [this](const httplib::Request& req, httplib::Response& res) {
        handleCreateStream(req, res);
    });
    
    server_->Patch("/api/v1/streams/:id", [this](const httplib::Request& req, httplib::Response& res) {
        handleUpdateStream(req, res);
    });
    
    server_->Delete("/api/v1/streams/:id", [this](const httplib::Request& req, httplib::Response& res) {
        handleDeleteStream(req, res);
    });
    
    server_->Post("/api/v1/streams/:id/start", [this](const httplib::Request& req, httplib::Response& res) {
        handleStartStream(req, res);
    });
    
    server_->Post("/api/v1/streams/:id/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleStopStream(req, res);
    });
    
    // Phasor endpoints (Module 2)
    server_->Post("/api/v1/phasors/:streamId", [this](const httplib::Request& req, httplib::Response& res) {
        handleUpdatePhasors(req, res);
    });
    
    server_->Post("/api/v1/phasors/:streamId/harmonics", [this](const httplib::Request& req, httplib::Response& res) {
        handleUpdateHarmonics(req, res);
    });
    
    // COMTRADE playback endpoint (Module 1)
    server_->Post("/api/v1/comtrade/playback", [this](const httplib::Request& req, httplib::Response& res) {
        handleComtradePlayback(req, res);
    });
    
    // Sequence endpoints (Module 3)
    server_->Post("/api/v1/sequences/run", [this](const httplib::Request& req, httplib::Response& res) {
        handleSequenceRun(req, res);
    });
    
    server_->Post("/api/v1/sequences/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleSequenceStop(req, res);
    });
    
    server_->Get("/api/v1/sequences/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleSequenceStatus(req, res);
    });
    
    server_->Post("/api/v1/sequences/pause", [this](const httplib::Request& req, httplib::Response& res) {
        handleSequencePause(req, res);
    });
    
    server_->Post("/api/v1/sequences/resume", [this](const httplib::Request& req, httplib::Response& res) {
        handleSequenceResume(req, res);
    });
    
    // GOOSE endpoints (Module 4)
    server_->Get("/api/v1/goose/subscriptions", [this](const httplib::Request& req, httplib::Response& res) {
        handleGooseGetSubscriptions(req, res);
    });
    
    server_->Post("/api/v1/goose/scan", [this](const httplib::Request& req, httplib::Response& res) {
        handleGooseScan(req, res);
    });
    
    server_->Post("/api/v1/goose/config", [this](const httplib::Request& req, httplib::Response& res) {
        handleGooseConfig(req, res);
    });
    
    // Analyzer endpoint (Module 5)
    server_->Post("/api/v1/analyzer/select", [this](const httplib::Request& req, httplib::Response& res) {
        handleAnalyzerSelect(req, res);
    });
    
    server_->Post("/api/v1/analyzer/stop", [this](const httplib::Request& req, httplib::Response& res) {
        handleAnalyzerStop(req, res);
    });
    
    server_->Get("/api/v1/analyzer/status", [this](const httplib::Request& req, httplib::Response& res) {
        handleAnalyzerStatus(req, res);
    });
    
    // Impedance injection endpoint (Module 6)
    server_->Post("/api/v1/impedance/apply", [this](const httplib::Request& req, httplib::Response& res) {
        handleImpedanceApply(req, res);
    });
    
    // Ramping test endpoint (Module 7)
    server_->Post("/api/v1/ramp/run", [this](const httplib::Request& req, httplib::Response& res) {
        handleRampRun(req, res);
    });
    
    // Distance relay test endpoint (Module 9)
    server_->Post("/api/v1/distance/run", [this](const httplib::Request& req, httplib::Response& res) {
        handleDistanceRun(req, res);
    });
    
    // Overcurrent test endpoint (Module 10)
    server_->Post("/api/v1/overcurrent/run", [this](const httplib::Request& req, httplib::Response& res) {
        handleOvercurrentRun(req, res);
    });
    
    // Differential test endpoint (Module 11)
    server_->Post("/api/v1/differential/run", [this](const httplib::Request& req, httplib::Response& res) {
        handleDifferentialRun(req, res);
    });
    
    // System/Configuration endpoints
    server_->Get("/api/v1/system/network-interfaces", [this](const httplib::Request& req, httplib::Response& res) {
        handleGetNetworkInterfaces(req, res);
    });
}

void HTTPServer::start() {
    if (running_.load()) {
        std::cerr << "HTTP server already running" << std::endl;
        return;
    }
    
    running_.store(true);
    
    serverThread_ = std::thread([this]() {
        std::cout << "HTTP server starting on port " << port_ << std::endl;
        if (!server_->listen("0.0.0.0", port_)) {
            std::cerr << "Failed to start HTTP server on port " << port_ << std::endl;
            running_.store(false);
        }
    });
    
    std::cout << "HTTP server started on port " << port_ << std::endl;
}

void HTTPServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_.store(false);
    server_->stop();
    
    if (serverThread_.joinable()) {
        serverThread_.join();
    }
    
    std::cout << "HTTP server stopped" << std::endl;
}

void HTTPServer::setSVPublisherManager(std::shared_ptr<SVPublisherManager> manager) {
    svManager_ = manager;
}

void HTTPServer::setSequenceEngine(std::shared_ptr<vts::sequence::SequenceEngine> engine) {
    sequenceEngine_ = engine;
}

void HTTPServer::setGooseSubscriber(std::shared_ptr<GooseSubscriber> subscriber) {
    gooseSubscriber_ = subscriber;
}

void HTTPServer::setAnalyzerEngine(std::shared_ptr<vts::analyzer::AnalyzerEngine> analyzer) {
    analyzerEngine_ = analyzer;
}

void HTTPServer::setWSServer(WSServer* wsServer) {
    wsServer_ = wsServer;
}

void HTTPServer::setImpedanceCalculator(std::shared_ptr<vts::testers::ImpedanceCalculator> calculator) {
    impedanceCalculator_ = calculator;
}

void HTTPServer::setRampingTester(std::shared_ptr<vts::testers::RampingTester> tester) {
    rampingTester_ = tester;
}

void HTTPServer::setDistanceTester(std::shared_ptr<vts::testers::DistanceTester> tester) {
    distanceTester_ = tester;
}

void HTTPServer::setOvercurrentTester(std::shared_ptr<vts::testers::OvercurrentTester> tester) {
    overcurrentTester_ = tester;
}

void HTTPServer::setDifferentialTester(std::shared_ptr<vts::testers::DifferentialTester> tester) {
    differentialTester_ = tester;
}

// Health endpoint
void HTTPServer::handleHealth(const httplib::Request& /*req*/, httplib::Response& res) {
    json response = {
        {"status", "ok"},
        {"timestamp", std::time(nullptr)},
        {"version", "1.0.0"}
    };
    sendJsonResponse(res, 200, response);
}

// Stream management endpoints
void HTTPServer::handleGetStreams(const httplib::Request& /*req*/, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    try {
        json streams = svManager_->listStreams();
        sendJsonResponse(res, 200, streams);
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to list streams: ") + e.what());
    }
}

void HTTPServer::handleCreateStream(const httplib::Request& req, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    try {
        json body = json::parse(req.body);
        
        // TODO: Validate against stream-config.schema.json
        std::string streamId = svManager_->createStream(body);
        
        json response = {
            {"id", streamId},
            {"message", "Stream created successfully"}
        };
        sendJsonResponse(res, 201, response);
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to create stream: ") + e.what());
    }
}

void HTTPServer::handleUpdateStream(const httplib::Request& req, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    std::string streamId = req.path_params.at("id");
    
    try {
        json body = json::parse(req.body);
        svManager_->updateStream(streamId, body);
        
        json response = {
            {"id", streamId},
            {"message", "Stream updated successfully"}
        };
        sendJsonResponse(res, 200, response);
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to update stream: ") + e.what());
    }
}

void HTTPServer::handleDeleteStream(const httplib::Request& req, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    std::string streamId = req.path_params.at("id");
    
    try {
        svManager_->deleteStream(streamId);
        
        json response = {
            {"id", streamId},
            {"message", "Stream deleted successfully"}
        };
        sendJsonResponse(res, 200, response);
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to delete stream: ") + e.what());
    }
}

void HTTPServer::handleStartStream(const httplib::Request& req, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    std::string streamId = req.path_params.at("id");
    
    try {
        svManager_->startStream(streamId);
        
        json response = {
            {"id", streamId},
            {"message", "Stream started successfully"}
        };
        sendJsonResponse(res, 200, response);
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to start stream: ") + e.what());
    }
}

void HTTPServer::handleStopStream(const httplib::Request& req, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    std::string streamId = req.path_params.at("id");
    
    try {
        svManager_->stopStream(streamId);
        
        json response = {
            {"id", streamId},
            {"message", "Stream stopped successfully"}
        };
        sendJsonResponse(res, 200, response);
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to stop stream: ") + e.what());
    }
}

// Phasor endpoints
void HTTPServer::handleUpdatePhasors(const httplib::Request& req, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    std::string streamId = req.path_params.at("streamId");
    
    try {
        json body = json::parse(req.body);
        svManager_->updatePhasors(streamId, body);
        
        sendJsonResponse(res, 200, {{"message", "Phasors updated"}});
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to update phasors: ") + e.what());
    }
}

void HTTPServer::handleUpdateHarmonics(const httplib::Request& req, httplib::Response& res) {
    if (!svManager_) {
        sendErrorResponse(res, 503, "Publisher manager not initialized");
        return;
    }
    
    std::string streamId = req.path_params.at("streamId");
    
    try {
        json body = json::parse(req.body);
        svManager_->updateHarmonics(streamId, body);
        
        sendJsonResponse(res, 200, {{"message", "Harmonics updated"}});
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to update harmonics: ") + e.what());
    }
}

// COMTRADE playback endpoint
void HTTPServer::handleComtradePlayback(const httplib::Request& /*req*/, httplib::Response& res) {
    // TODO: Handle multipart/form-data file upload
    sendErrorResponse(res, 501, "COMTRADE playback not yet implemented");
}

// Sequence endpoints
void HTTPServer::handleSequenceRun(const httplib::Request& req, httplib::Response& res) {
    if (!sequenceEngine_) {
        sendErrorResponse(res, 503, "Sequence engine not initialized");
        return;
    }
    
    try {
        json body = json::parse(req.body);
        
        // Parse sequence from JSON
        vts::sequence::Sequence seq;
        
        // Parse active streams
        if (!body.contains("activeStreams") || !body["activeStreams"].is_array()) {
            sendErrorResponse(res, 400, "Missing or invalid 'activeStreams' field");
            return;
        }
        
        for (const auto& streamId : body["activeStreams"]) {
            if (!streamId.is_string()) {
                sendErrorResponse(res, 400, "Stream IDs must be strings");
                return;
            }
            seq.activeStreams.push_back(streamId.get<std::string>());
        }
        
        // Parse states
        if (!body.contains("states") || !body["states"].is_array()) {
            sendErrorResponse(res, 400, "Missing or invalid 'states' field");
            return;
        }
        
        for (const auto& stateJson : body["states"]) {
            vts::sequence::SequenceState state;
            
            // Parse state name
            if (!stateJson.contains("name") || !stateJson["name"].is_string()) {
                sendErrorResponse(res, 400, "State missing 'name' field");
                return;
            }
            state.name = stateJson["name"].get<std::string>();
            
            // Parse duration
            if (!stateJson.contains("durationSec") || !stateJson["durationSec"].is_number()) {
                sendErrorResponse(res, 400, "State missing 'durationSec' field");
                return;
            }
            state.durationSec = stateJson["durationSec"].get<double>();
            
            // Parse transition
            if (!stateJson.contains("transition") || !stateJson["transition"].is_object()) {
                sendErrorResponse(res, 400, "State missing 'transition' field");
                return;
            }
            
            std::string transitionType = stateJson["transition"]["type"].get<std::string>();
            if (transitionType == "time") {
                state.transition = vts::sequence::StateTransition(vts::sequence::TransitionType::TIME);
            } else if (transitionType == "goose_trip") {
                state.transition = vts::sequence::StateTransition(vts::sequence::TransitionType::GOOSE_TRIP);
            } else {
                sendErrorResponse(res, 400, "Invalid transition type: " + transitionType);
                return;
            }
            
            // Parse phasors
            if (!stateJson.contains("phasors") || !stateJson["phasors"].is_object()) {
                sendErrorResponse(res, 400, "State missing 'phasors' field");
                return;
            }
            
            for (const auto& [streamId, streamPhasors] : stateJson["phasors"].items()) {
                vts::sequence::StreamPhasorState phasorState;
                
                // Parse frequency
                if (streamPhasors.contains("freq") && streamPhasors["freq"].is_number()) {
                    phasorState.freq = streamPhasors["freq"].get<double>();
                }
                
                // Parse channels
                if (streamPhasors.contains("channels") && streamPhasors["channels"].is_object()) {
                    for (const auto& [channelId, channelData] : streamPhasors["channels"].items()) {
                        if (channelData.contains("mag") && channelData.contains("angleDeg") &&
                            channelData["mag"].is_number() && channelData["angleDeg"].is_number()) {
                            double mag = channelData["mag"].get<double>();
                            double angle = channelData["angleDeg"].get<double>();
                            phasorState.channels[channelId] = vts::sequence::ChannelPhasor(mag, angle);
                        }
                    }
                }
                
                state.phasors[streamId] = phasorState;
            }
            
            seq.states.push_back(state);
        }
        
        // Start the sequence
        if (!sequenceEngine_->start(seq)) {
            sendErrorResponse(res, 400, sequenceEngine_->getLastError());
            return;
        }
        
        json response = {
            {"message", "Sequence started successfully"},
            {"stateCount", seq.states.size()},
            {"activeStreams", seq.activeStreams}
        };
        sendJsonResponse(res, 200, response);
        
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to start sequence: ") + e.what());
    }
}

void HTTPServer::handleSequenceStop(const httplib::Request& /*req*/, httplib::Response& res) {
    if (!sequenceEngine_) {
        sendErrorResponse(res, 503, "Sequence engine not initialized");
        return;
    }
    
    try {
        sequenceEngine_->stop();
        
        json response = {
            {"message", "Sequence stopped successfully"}
        };
        sendJsonResponse(res, 200, response);
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to stop sequence: ") + e.what());
    }
}

void HTTPServer::handleSequenceStatus(const httplib::Request& /*req*/, httplib::Response& res) {
    if (!sequenceEngine_) {
        sendErrorResponse(res, 503, "Sequence engine not initialized");
        return;
    }
    
    try {
        auto status = sequenceEngine_->getStatus();
        std::string statusStr;
        
        switch (status) {
            case vts::sequence::SequenceStatus::IDLE:
                statusStr = "idle";
                break;
            case vts::sequence::SequenceStatus::RUNNING:
                statusStr = "running";
                break;
            case vts::sequence::SequenceStatus::PAUSED:
                statusStr = "paused";
                break;
            case vts::sequence::SequenceStatus::COMPLETED:
                statusStr = "completed";
                break;
            case vts::sequence::SequenceStatus::STOPPED:
                statusStr = "stopped";
                break;
            case vts::sequence::SequenceStatus::ERROR:
                statusStr = "error";
                break;
        }
        
        json response = {
            {"status", statusStr},
            {"currentState", sequenceEngine_->getCurrentStateIndex()},
            {"stateElapsed", sequenceEngine_->getStateElapsedTime()},
            {"totalElapsed", sequenceEngine_->getTotalElapsedTime()}
        };
        
        if (status == vts::sequence::SequenceStatus::ERROR) {
            response["error"] = sequenceEngine_->getLastError();
        }
        
        sendJsonResponse(res, 200, response);
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to get sequence status: ") + e.what());
    }
}

void HTTPServer::handleSequencePause(const httplib::Request& /*req*/, httplib::Response& res) {
    if (!sequenceEngine_) {
        sendErrorResponse(res, 503, "Sequence engine not initialized");
        return;
    }
    
    try {
        sequenceEngine_->pause();
        
        json response = {
            {"message", "Sequence paused successfully"}
        };
        sendJsonResponse(res, 200, response);
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to pause sequence: ") + e.what());
    }
}

void HTTPServer::handleSequenceResume(const httplib::Request& /*req*/, httplib::Response& res) {
    if (!sequenceEngine_) {
        sendErrorResponse(res, 503, "Sequence engine not initialized");
        return;
    }
    
    try {
        sequenceEngine_->resume();
        
        json response = {
            {"message", "Sequence resumed successfully"}
        };
        sendJsonResponse(res, 200, response);
        
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Failed to resume sequence: ") + e.what());
    }
}

// GOOSE endpoints
void HTTPServer::handleGooseGetSubscriptions(const httplib::Request& /*req*/, httplib::Response& res) {
    // TODO: Return actual GOOSE subscriptions from GOOSE engine
    // For now, return empty array to prevent 404 errors in frontend
    sendJsonResponse(res, 200, {{"subscriptions", json::array()}});
}

void HTTPServer::handleGooseScan(const httplib::Request& /*req*/, httplib::Response& res) {
    // TODO: Scan for GOOSE messages
    sendJsonResponse(res, 200, {{"entries", json::array()}});
}

void HTTPServer::handleGooseConfig(const httplib::Request& req, httplib::Response& res) {
    try {
        json body = json::parse(req.body);
        
        // TODO: Configure GOOSE subscription
        
        sendJsonResponse(res, 200, {{"message", "GOOSE configured"}});
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    }
}

// Analyzer endpoint
void HTTPServer::handleAnalyzerSelect(const httplib::Request& req, httplib::Response& res) {
    if (!analyzerEngine_) {
        sendErrorResponse(res, 503, "Analyzer engine not available");
        return;
    }

    try {
        json body = json::parse(req.body);
        
        // Validate required fields
        if (!body.contains("streamMac")) {
            sendErrorResponse(res, 400, "Missing streamMac field");
            return;
        }
        
        if (!body.contains("sampleRate")) {
            sendErrorResponse(res, 400, "Missing sampleRate field");
            return;
        }
        
        std::string streamMac = body["streamMac"];
        int sampleRate = body["sampleRate"];
        
        // Validate MAC address format
        if (streamMac.length() != 17) {
            sendErrorResponse(res, 400, "Invalid MAC address format");
            return;
        }
        
        // Validate sample rate
        if (sampleRate <= 0 || sampleRate > 100000) {
            sendErrorResponse(res, 400, "Invalid sample rate");
            return;
        }
        
        // Start analyzer
        bool success = analyzerEngine_->start(streamMac, sampleRate);
        
        if (success) {
            sendJsonResponse(res, 200, {
                {"message", "Analyzer started"},
                {"streamMac", streamMac},
                {"sampleRate", sampleRate}
            });
        } else {
            sendErrorResponse(res, 500, analyzerEngine_->getLastError());
        }
        
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    }
}

void HTTPServer::handleAnalyzerStop(const httplib::Request& /*req*/, httplib::Response& res) {
    if (!analyzerEngine_) {
        sendErrorResponse(res, 503, "Analyzer engine not available");
        return;
    }
    
    analyzerEngine_->stop();
    
    sendJsonResponse(res, 200, {
        {"message", "Analyzer stopped"}
    });
}

void HTTPServer::handleAnalyzerStatus(const httplib::Request& /*req*/, httplib::Response& res) {
    if (!analyzerEngine_) {
        sendErrorResponse(res, 503, "Analyzer engine not available");
        return;
    }
    
    bool running = analyzerEngine_->isRunning();
    std::string streamMac = analyzerEngine_->getStreamMac();
    
    sendJsonResponse(res, 200, {
        {"running", running},
        {"streamMac", streamMac}
    });
}


// Impedance injection endpoint
void HTTPServer::handleImpedanceApply(const httplib::Request& req, httplib::Response& res) {
    if (!impedanceCalculator_) {
        sendErrorResponse(res, 503, "Impedance calculator not initialized");
        return;
    }
    
    if (!svManager_) {
        sendErrorResponse(res, 503, "SV publisher manager not initialized");
        return;
    }
    
    try {
        json body = json::parse(req.body);
        
        // Parse impedance parameters
        std::string faultTypeStr = body.value("faultType", "ABC");
        double R = body.value("R", 0.0);
        double X = body.value("X", 0.0);
        
        SourceImpedance source;
        source.RS1 = body.value("RS1", 1.0);
        source.XS1 = body.value("XS1", 10.0);
        source.RS0 = body.value("RS0", 3.0);
        source.XS0 = body.value("XS0", 30.0);
        source.Vprefault = body.value("Vprefault", 66395.0); // 115kV/sqrt(3)
        
        // Parse fault type
        FaultType faultType = impedanceCalculator_->parseFaultType(faultTypeStr);
        
        // Create fault impedance structure
        vts::testers::FaultImpedance faultZ;
        faultZ.R = R;
        faultZ.X = X;
        
        // Calculate phasors
        PhasorState state = impedanceCalculator_->calculateFault(
            faultType, faultZ, source
        );
        
        // Apply to stream if specified
        std::string streamId = body.value("streamId", "");
        if (!streamId.empty()) {
            // Update stream phasors
            json phasorUpdate = {
                {"voltage", {
                    {"A", {{"magnitude", std::abs(state.voltage.A)}, {"angle", std::arg(state.voltage.A) * 180.0 / M_PI}}},
                    {"B", {{"magnitude", std::abs(state.voltage.B)}, {"angle", std::arg(state.voltage.B) * 180.0 / M_PI}}},
                    {"C", {{"magnitude", std::abs(state.voltage.C)}, {"angle", std::arg(state.voltage.C) * 180.0 / M_PI}}}
                }},
                {"current", {
                    {"A", {{"magnitude", std::abs(state.current.A)}, {"angle", std::arg(state.current.A) * 180.0 / M_PI}}},
                    {"B", {{"magnitude", std::abs(state.current.B)}, {"angle", std::arg(state.current.B) * 180.0 / M_PI}}},
                    {"C", {{"magnitude", std::abs(state.current.C)}, {"angle", std::arg(state.current.C) * 180.0 / M_PI}}}
                }}
            };
            
            svManager_->updateStream(streamId, phasorUpdate);
        }
        
        // Return calculated phasors
        json response = {
            {"voltage", {
                {"A", {{"magnitude", std::abs(state.voltage.A)}, {"angle", std::arg(state.voltage.A) * 180.0 / M_PI}}},
                {"B", {{"magnitude", std::abs(state.voltage.B)}, {"angle", std::arg(state.voltage.B) * 180.0 / M_PI}}},
                {"C", {{"magnitude", std::abs(state.voltage.C)}, {"angle", std::arg(state.voltage.C) * 180.0 / M_PI}}}
            }},
            {"current", {
                {"A", {{"magnitude", std::abs(state.current.A)}, {"angle", std::arg(state.current.A) * 180.0 / M_PI}}},
                {"B", {{"magnitude", std::abs(state.current.B)}, {"angle", std::arg(state.current.B) * 180.0 / M_PI}}},
                {"C", {{"magnitude", std::abs(state.current.C)}, {"angle", std::arg(state.current.C) * 180.0 / M_PI}}}
            }},
            {"faultType", faultTypeStr},
            {"R", R},
            {"X", X}
        };
        
        sendJsonResponse(res, 200, response);
        
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Impedance calculation failed: ") + e.what());
    }
}

// Ramping test endpoint
void HTTPServer::handleRampRun(const httplib::Request& req, httplib::Response& res) {
    if (!rampingTester_) {
        sendErrorResponse(res, 503, "Ramping tester not initialized");
        return;
    }
    
    if (!svManager_) {
        sendErrorResponse(res, 503, "SV publisher manager not initialized");
        return;
    }
    
    try {
        json body = json::parse(req.body);
        
        // Parse configuration
        RampConfig config;
        config.variable = rampingTester_->parseVariable(body.value("variable", "VOLTAGE_3PH"));
        config.startValue = body.value("startValue", 0.0);
        config.endValue = body.value("endValue", 150.0);
        config.stepSize = body.value("stepSize", 0.1);
        config.stepDuration = body.value("stepDuration", 0.05);
        config.monitorTrip = body.value("monitorTrip", true);
        config.streamId = body.value("streamId", "");
        
        // Set up callbacks
        rampingTester_->setTripFlagGetter([]() {
            return vts::isTripFlagSet();
        });
        
        rampingTester_->setValueSetter([this, config](RampVariable var, double value) {
            if (config.streamId.empty()) return;
            
            // Update stream based on variable type
            json update;
            switch (var) {
                case RampVariable::VOLTAGE_A:
                    update["voltage"]["A"]["magnitude"] = value;
                    break;
                case RampVariable::VOLTAGE_B:
                    update["voltage"]["B"]["magnitude"] = value;
                    break;
                case RampVariable::VOLTAGE_C:
                    update["voltage"]["C"]["magnitude"] = value;
                    break;
                case RampVariable::VOLTAGE_3PH:
                    update["voltage"]["A"]["magnitude"] = value;
                    update["voltage"]["B"]["magnitude"] = value;
                    update["voltage"]["C"]["magnitude"] = value;
                    break;
                case RampVariable::CURRENT_A:
                    update["current"]["A"]["magnitude"] = value;
                    break;
                case RampVariable::CURRENT_B:
                    update["current"]["B"]["magnitude"] = value;
                    break;
                case RampVariable::CURRENT_C:
                    update["current"]["C"]["magnitude"] = value;
                    break;
                case RampVariable::CURRENT_3PH:
                    update["current"]["A"]["magnitude"] = value;
                    update["current"]["B"]["magnitude"] = value;
                    update["current"]["C"]["magnitude"] = value;
                    break;
                case RampVariable::FREQUENCY:
                    update["frequency"] = value;
                    break;
            }
            
            svManager_->updateStream(config.streamId, update);
        });
        
        // Run the ramp test
        RampResult result = rampingTester_->run(config);
        
        // Return results
        json response = {
            {"pickupValue", result.pickupValue},
            {"dropoffValue", result.dropoffValue},
            {"resetRatio", result.resetRatio},
            {"pickupTime", result.pickupTime},
            {"dropoffTime", result.dropoffTime},
            {"completed", result.completed},
            {"error", result.error}
        };
        
        sendJsonResponse(res, 200, response);
        
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Ramping test failed: ") + e.what());
    }
}

// Distance relay test endpoint
void HTTPServer::handleDistanceRun(const httplib::Request& req, httplib::Response& res) {
    if (!distanceTester_) {
        sendErrorResponse(res, 503, "Distance tester not initialized");
        return;
    }
    
    if (!svManager_) {
        sendErrorResponse(res, 503, "SV publisher manager not initialized");
        return;
    }
    
    try {
        json body = json::parse(req.body);
        
        // Parse test configuration
        DistanceTestConfig config;
        
        // Parse source impedance
        if (body.contains("source")) {
            auto source = body["source"];
            config.source.RS1 = source.value("RS1", 1.0);
            config.source.XS1 = source.value("XS1", 10.0);
            config.source.RS0 = source.value("RS0", 3.0);
            config.source.XS0 = source.value("XS0", 30.0);
            config.source.Vprefault = source.value("Vprefault", 66395.0);
        }
        
        // Parse test points
        if (body.contains("points") && body["points"].is_array()) {
            for (const auto& pt : body["points"]) {
                DistancePoint point;
                point.R = pt.value("R", 0.0);
                point.X = pt.value("X", 0.0);
                point.faultType = impedanceCalculator_->parseFaultType(pt.value("faultType", "ABC"));
                point.expectedTime = pt.value("expectedTime", 0.0);
                point.label = pt.value("label", "");
                config.points.push_back(point);
            }
        }
        
        // Parse timing parameters
        config.prefaultDuration = body.value("prefaultDuration", 1.0);
        config.faultDuration = body.value("faultDuration", 5.0);
        config.timeTolerance = body.value("timeTolerance", 0.05);
        config.stopOnFirstFailure = body.value("stopOnFirstFailure", false);
        config.streamId = body.value("streamId", "");
        
        // Set up callbacks
        distanceTester_->setTripFlagGetter([]() {
            return vts::isTripFlagSet();
        });
        
        distanceTester_->setPhasorSetter([this, config](const PhasorState& state) {
            if (config.streamId.empty()) return;
            
            json update = {
                {"voltage", {
                    {"A", {{"magnitude", std::abs(state.voltage.A)}, {"angle", std::arg(state.voltage.A) * 180.0 / M_PI}}},
                    {"B", {{"magnitude", std::abs(state.voltage.B)}, {"angle", std::arg(state.voltage.B) * 180.0 / M_PI}}},
                    {"C", {{"magnitude", std::abs(state.voltage.C)}, {"angle", std::arg(state.voltage.C) * 180.0 / M_PI}}}
                }},
                {"current", {
                    {"A", {{"magnitude", std::abs(state.current.A)}, {"angle", std::arg(state.current.A) * 180.0 / M_PI}}},
                    {"B", {{"magnitude", std::abs(state.current.B)}, {"angle", std::arg(state.current.B) * 180.0 / M_PI}}},
                    {"C", {{"magnitude", std::abs(state.current.C)}, {"angle", std::arg(state.current.C) * 180.0 / M_PI}}}
                }}
            };
            
            svManager_->updateStream(config.streamId, update);
        });
        
        // Run the distance test
        auto results = distanceTester_->run(config);
        
        // Format results
        json resultsJson = json::array();
        for (const auto& result : results) {
            resultsJson.push_back({
                {"R", result.R},
                {"X", result.X},
                {"tripped", result.tripped},
                {"tripTime", result.tripTime},
                {"passed", result.passed},
                {"error", result.error}
            });
        }
        
        json response = {
            {"results", resultsJson},
            {"totalPoints", results.size()},
            {"passed", std::all_of(results.begin(), results.end(), 
                [](const DistanceResult& r) { return r.passed; })}
        };
        
        sendJsonResponse(res, 200, response);
        
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Distance test failed: ") + e.what());
    }
}

// Overcurrent test endpoint
void HTTPServer::handleOvercurrentRun(const httplib::Request& req, httplib::Response& res) {
    if (!overcurrentTester_) {
        sendErrorResponse(res, 503, "Overcurrent tester not initialized");
        return;
    }
    
    if (!svManager_) {
        sendErrorResponse(res, 503, "SV publisher manager not initialized");
        return;
    }
    
    try {
        json body = json::parse(req.body);
        
        // Parse overcurrent settings
        OCTestConfig config;
        
        if (body.contains("settings")) {
            auto settings = body["settings"];
            config.settings.pickupCurrent = settings.value("pickupCurrent", 100.0);
            config.settings.TMS = settings.value("TMS", 0.1);
            config.settings.curve = overcurrentTester_->parseCurve(settings.value("curve", "STANDARD_INVERSE"));
        }
        
        // Parse test points
        if (body.contains("points") && body["points"].is_array()) {
            for (const auto& pt : body["points"]) {
                OCPoint point;
                point.currentMultiple = pt.value("currentMultiple", 2.0);
                point.expectedTime = pt.value("expectedTime", 0.0);
                point.label = pt.value("label", "");
                config.points.push_back(point);
            }
        }
        
        // Parse tolerance
        config.timeTolerance = body.value("timeTolerance", 5.0);
        config.toleranceIsPercent = body.value("toleranceIsPercent", true);
        config.maxTestDuration = body.value("maxTestDuration", 60.0);
        config.stopOnFirstFailure = body.value("stopOnFirstFailure", false);
        config.streamId = body.value("streamId", "");
        
        // Set up callbacks
        overcurrentTester_->setTripFlagGetter([]() {
            return vts::isTripFlagSet();
        });
        
        overcurrentTester_->setCurrentSetter([this, config](double current) {
            if (config.streamId.empty()) return;
            
            json update = {
                {"current", {
                    {"A", {{"magnitude", current}}},
                    {"B", {{"magnitude", current}}},
                    {"C", {{"magnitude", current}}}
                }}
            };
            
            svManager_->updateStream(config.streamId, update);
        });
        
        // Run the overcurrent test
        auto results = overcurrentTester_->run(config);
        
        // Format results
        json resultsJson = json::array();
        for (const auto& result : results) {
            resultsJson.push_back({
                {"currentMultiple", result.currentMultiple},
                {"actualCurrent", result.actualCurrent},
                {"expectedTime", result.expectedTime},
                {"measuredTime", result.measuredTime},
                {"tripped", result.tripped},
                {"passed", result.passed},
                {"error", result.error}
            });
        }
        
        json response = {
            {"results", resultsJson},
            {"totalPoints", results.size()},
            {"passed", std::all_of(results.begin(), results.end(), 
                [](const OCResult& r) { return r.passed; })},
            {"settings", {
                {"pickupCurrent", config.settings.pickupCurrent},
                {"TMS", config.settings.TMS},
                {"curve", overcurrentTester_->curveToString(config.settings.curve)}
            }}
        };
        
        sendJsonResponse(res, 200, response);
        
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Overcurrent test failed: ") + e.what());
    }
}

// Differential test endpoint
void HTTPServer::handleDifferentialRun(const httplib::Request& req, httplib::Response& res) {
    if (!differentialTester_) {
        sendErrorResponse(res, 503, "Differential tester not initialized");
        return;
    }
    
    if (!svManager_) {
        sendErrorResponse(res, 503, "SV publisher manager not initialized");
        return;
    }
    
    try {
        json body = json::parse(req.body);
        
        // Parse differential test configuration
        DifferentialTestConfig config;
        
        // Parse test points
        if (body.contains("points") && body["points"].is_array()) {
            for (const auto& pt : body["points"]) {
                DifferentialPoint point;
                point.Ir = pt.value("Ir", 0.0);
                point.Id = pt.value("Id", 0.0);
                point.expectedTime = pt.value("expectedTime", 0.0);
                point.label = pt.value("label", "");
                config.points.push_back(point);
            }
        }
        
        // Parse configuration
        config.timeTolerance = body.value("timeTolerance", 0.05);
        config.maxTestDuration = body.value("maxTestDuration", 5.0);
        config.stopOnFirstFailure = body.value("stopOnFirstFailure", false);
        config.stream1Id = body.value("stream1Id", "");
        config.stream2Id = body.value("stream2Id", "");
        
        // Set up callbacks
        differentialTester_->setTripFlagGetter([]() {
            return vts::isTripFlagSet();
        });
        
        differentialTester_->setSide1CurrentSetter([this, config](double current) {
            if (config.stream1Id.empty()) return;
            
            json update = {
                {"current", {
                    {"A", {{"magnitude", current}}},
                    {"B", {{"magnitude", current}}},
                    {"C", {{"magnitude", current}}}
                }}
            };
            
            svManager_->updateStream(config.stream1Id, update);
        });
        
        differentialTester_->setSide2CurrentSetter([this, config](double current) {
            if (config.stream2Id.empty()) return;
            
            json update = {
                {"current", {
                    {"A", {{"magnitude", current}}},
                    {"B", {{"magnitude", current}}},
                    {"C", {{"magnitude", current}}}
                }}
            };
            
            svManager_->updateStream(config.stream2Id, update);
        });
        
        // Run the differential test
        auto results = differentialTester_->run(config);
        
        // Format results
        json resultsJson = json::array();
        for (const auto& result : results) {
            resultsJson.push_back({
                {"Ir", result.Ir},
                {"Id", result.Id},
                {"Is1", result.Is1},
                {"Is2", result.Is2},
                {"expectedTime", result.expectedTime},
                {"tripTime", result.tripTime},
                {"tripped", result.tripped},
                {"passed", result.passed},
                {"error", result.error}
            });
        }
        
        json response = {
            {"results", resultsJson},
            {"totalPoints", results.size()},
            {"passed", std::all_of(results.begin(), results.end(), 
                [](const DifferentialResult& r) { return r.passed; })}
        };
        
        sendJsonResponse(res, 200, response);
        
    } catch (const json::exception& e) {
        sendErrorResponse(res, 400, std::string("Invalid JSON: ") + e.what());
    } catch (const std::exception& e) {
        sendErrorResponse(res, 500, std::string("Differential test failed: ") + e.what());
    }
}

// Utility functions
void HTTPServer::sendJsonResponse(httplib::Response& res, int status, const json& data) {
    res.status = status;
    res.set_content(data.dump(), "application/json");
}

void HTTPServer::sendErrorResponse(httplib::Response& res, int status, const std::string& message) {
    json error = {
        {"error", message},
        {"timestamp", std::time(nullptr)}
    };
    sendJsonResponse(res, status, error);
}

void HTTPServer::handleGetNetworkInterfaces(const httplib::Request& /*req*/, httplib::Response& res) {
#ifdef VTS_PLATFORM_MAC
    try {
        std::cout << "[HTTP] Getting network interfaces..." << std::endl;
        
        // Get detailed interface information on macOS
        auto interfaces = vts::platform::getNetworkInterfacesDetailed();
        
        std::cout << "[HTTP] Found " << interfaces.size() << " interfaces" << std::endl;
        
        json interfacesList = json::array();
        for (const auto& iface : interfaces) {
            json ifaceJson = {
                {"name", iface.name},
                {"active", iface.isActive},
                {"macAddress", iface.macAddress.empty() ? json() : json(iface.macAddress)},
                {"ipAddress", iface.ipAddress.empty() ? json() : json(iface.ipAddress)}
            };
            interfacesList.push_back(ifaceJson);
        }
        
        json response = {
            {"interfaces", interfacesList},
            {"platform", "macOS"}
        };
        
        std::cout << "[HTTP] Sending response..." << std::endl;
        sendJsonResponse(res, 200, response);
    } catch (const std::exception& e) {
        std::cerr << "[HTTP] Error getting network interfaces: " << e.what() << std::endl;
        sendErrorResponse(res, 500, std::string("Failed to get network interfaces: ") + e.what());
    }
#else
    // Stub implementation for other platforms
    json response = {
        {"interfaces", json::array()},
        {"platform", "unsupported"},
        {"message", "Network interface detection not implemented for this platform"}
    };
    
    sendJsonResponse(res, 200, response);
#endif
}

bool HTTPServer::validateJson(const json& /*data*/, const std::string& /*schemaName*/) {
    // TODO: Implement JSON schema validation
    return true;
}
