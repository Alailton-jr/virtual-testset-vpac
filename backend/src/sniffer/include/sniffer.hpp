

#ifndef SNIFFER_HPP
#define SNIFFER_HPP

#include <string>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <array>
#include <memory>

#include "general_definition.hpp"
#include "raw_socket_platform.hpp"
#include "thread_pool.hpp"
#include "trip_rule_evaluator.hpp"

// Forward declarations
class WSServer;

namespace vts {
namespace analyzer {
    class AnalyzerEngine;
}
}

#define WINDOW_STEP 0.2

struct Goose_info{
    std::string goCbRef;
    std::vector<uint8_t> mac_dst;
    std::vector<std::vector<uint8_t>> input;  // {Digital Input POS, GOOSE data pos}
};

void* SnifferThread(void* arg);

class SnifferClass {
public:
    std::atomic<bool> running;
    std::atomic<bool> stop;
    int noThreads;
    int noTasks;
    int priority;

    pthread_t thd;
    bool threadStarted;

    RawSocket socket;
    std::array<std::atomic<uint8_t>, 16>* digitalInput;
    std::vector<Goose_info> goInfo;
    
    // Trip rule evaluator for GOOSE-based trip conditions
    std::unique_ptr<vts::sniffer::TripRuleEvaluator> tripEvaluator;
    
    // WebSocket server for event emission (weak_ptr to avoid ownership issues)
    std::weak_ptr<WSServer> wsServer;
    
    // Analyzer engine for SV stream analysis (weak_ptr to avoid ownership issues)
    std::weak_ptr<vts::analyzer::AnalyzerEngine> analyzerEngine;

    SnifferClass() : running(false), stop(false), threadStarted(false) {
        tripEvaluator = std::make_unique<vts::sniffer::TripRuleEvaluator>();
    }
    
    ~SnifferClass(){
        stopThread();
    }

    void init(){
    }

    void startThread(std::vector<Goose_info> gooseInfo){
        if (threadStarted) {
            throw std::runtime_error("Sniffer thread already started");
        }

        this->goInfo = gooseInfo;
        this->noThreads = Sniffer_NoThreads;
        this->noTasks = Sniffer_NoTasks;
        this->priority = Sniffer_ThreadPriority;

        int ret = pthread_create(&this->thd, NULL, SnifferThread, static_cast<void*>(this));
        if (ret != 0) {
            throw std::runtime_error("Failed to create sniffer thread: " + std::string(strerror(ret)));
        }
        threadStarted = true;
        
        struct sched_param param;
        param.sched_priority = this->priority;
        pthread_setschedparam(this->thd, SCHED_FIFO, &param);
    }

    void stopThread(){
        if (!threadStarted) {
            return;
        }
        
        stop.store(true, std::memory_order_release);
        pthread_join(this->thd, NULL);
        threadStarted = false;
    }
    
    /**
     * @brief Set the WebSocket server for GOOSE event emission
     * 
     * @param server Shared pointer to WSServer
     */
    void setWebSocketServer(std::shared_ptr<WSServer> server) {
        wsServer = server;
    }
    
    /**
     * @brief Set the analyzer engine for SV stream analysis
     * 
     * @param analyzer Shared pointer to AnalyzerEngine
     */
    void setAnalyzerEngine(std::shared_ptr<vts::analyzer::AnalyzerEngine> analyzer) {
        analyzerEngine = analyzer;
    }

};


#endif // SNIFFER_HPP