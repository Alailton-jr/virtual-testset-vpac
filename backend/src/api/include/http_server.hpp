#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <httplib.h>
#include <nlohmann/json.hpp>
#include <memory>
#include <string>
#include <atomic>
#include <thread>

using json = nlohmann::json;

// Forward declarations
class SVPublisherManager;
class GooseSubscriber;

namespace vts {
namespace testers {
    class ImpedanceCalculator;
    class RampingTester;
    class DistanceTester;
    class OvercurrentTester;
    class DifferentialTester;
}
namespace analyzer {
    class AnalyzerEngine;
}
namespace sequence {
    class SequenceEngine;
}
}

class HTTPServer {
public:
    HTTPServer(int port);
    ~HTTPServer();

    // Server control
    void start();
    void stop();
    bool isRunning() const { return running_.load(); }

    // Set component references
    void setSVPublisherManager(std::shared_ptr<SVPublisherManager> manager);
    void setSequenceEngine(std::shared_ptr<vts::sequence::SequenceEngine> engine);
    void setGooseSubscriber(std::shared_ptr<GooseSubscriber> subscriber);
    void setAnalyzerEngine(std::shared_ptr<vts::analyzer::AnalyzerEngine> analyzer);
    void setWSServer(class WSServer* wsServer);
    
    // Set tester component references
    void setImpedanceCalculator(std::shared_ptr<vts::testers::ImpedanceCalculator> calculator);
    void setRampingTester(std::shared_ptr<vts::testers::RampingTester> tester);
    void setDistanceTester(std::shared_ptr<vts::testers::DistanceTester> tester);
    void setOvercurrentTester(std::shared_ptr<vts::testers::OvercurrentTester> tester);
    void setDifferentialTester(std::shared_ptr<vts::testers::DifferentialTester> tester);

private:
    // Setup route handlers
    void setupRoutes();
    
    // Health endpoint
    void handleHealth(const httplib::Request& req, httplib::Response& res);
    
    // Stream management endpoints (Module 13)
    void handleGetStreams(const httplib::Request& req, httplib::Response& res);
    void handleCreateStream(const httplib::Request& req, httplib::Response& res);
    void handleUpdateStream(const httplib::Request& req, httplib::Response& res);
    void handleDeleteStream(const httplib::Request& req, httplib::Response& res);
    void handleStartStream(const httplib::Request& req, httplib::Response& res);
    void handleStopStream(const httplib::Request& req, httplib::Response& res);
    
    // Phasor endpoints (Module 2)
    void handleUpdatePhasors(const httplib::Request& req, httplib::Response& res);
    void handleUpdateHarmonics(const httplib::Request& req, httplib::Response& res);
    
    // COMTRADE playback endpoints (Module 1)
    void handleComtradePlayback(const httplib::Request& req, httplib::Response& res);
    
    // Sequence endpoints (Module 3)
    void handleSequenceRun(const httplib::Request& req, httplib::Response& res);
    void handleSequenceStop(const httplib::Request& req, httplib::Response& res);
    void handleSequenceStatus(const httplib::Request& req, httplib::Response& res);
    void handleSequencePause(const httplib::Request& req, httplib::Response& res);
    void handleSequenceResume(const httplib::Request& req, httplib::Response& res);
    
    // GOOSE endpoints (Module 4)
    void handleGooseGetSubscriptions(const httplib::Request& req, httplib::Response& res);
    void handleGooseScan(const httplib::Request& req, httplib::Response& res);
    void handleGooseConfig(const httplib::Request& req, httplib::Response& res);
    
    // Analyzer endpoints (Module 5)
    void handleAnalyzerSelect(const httplib::Request& req, httplib::Response& res);
    void handleAnalyzerStop(const httplib::Request& req, httplib::Response& res);
    void handleAnalyzerStatus(const httplib::Request& req, httplib::Response& res);
    
    // Impedance injection endpoints (Module 6)
    void handleImpedanceApply(const httplib::Request& req, httplib::Response& res);
    
    // Ramping test endpoints (Module 7)
    void handleRampRun(const httplib::Request& req, httplib::Response& res);
    
    // Distance relay test endpoints (Module 9)
    void handleDistanceRun(const httplib::Request& req, httplib::Response& res);
    
    // Overcurrent test endpoints (Module 10)
    void handleOvercurrentRun(const httplib::Request& req, httplib::Response& res);
    
    // Differential test endpoints (Module 11)
    void handleDifferentialRun(const httplib::Request& req, httplib::Response& res);
    
    // System/Configuration endpoints
    void handleGetNetworkInterfaces(const httplib::Request& req, httplib::Response& res);
    
    // Utility functions
    void sendJsonResponse(httplib::Response& res, int status, const json& data);
    void sendErrorResponse(httplib::Response& res, int status, const std::string& message);
    bool validateJson(const json& data, const std::string& schemaName);

    int port_;
    std::atomic<bool> running_;
    std::unique_ptr<httplib::Server> server_;
    std::thread serverThread_;
    
    // Component references
    std::shared_ptr<SVPublisherManager> svManager_;
    std::shared_ptr<vts::sequence::SequenceEngine> sequenceEngine_;
    std::shared_ptr<GooseSubscriber> gooseSubscriber_;
    std::shared_ptr<vts::analyzer::AnalyzerEngine> analyzerEngine_;
    class WSServer* wsServer_;
    
    // Tester component references
    std::shared_ptr<vts::testers::ImpedanceCalculator> impedanceCalculator_;
    std::shared_ptr<vts::testers::RampingTester> rampingTester_;
    std::shared_ptr<vts::testers::DistanceTester> distanceTester_;
    std::shared_ptr<vts::testers::OvercurrentTester> overcurrentTester_;
    std::shared_ptr<vts::testers::DifferentialTester> differentialTester_;
};

#endif // HTTP_SERVER_HPP
