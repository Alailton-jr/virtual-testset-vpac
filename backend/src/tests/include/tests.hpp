#ifndef TESTS_HPP
#define TESTS_HPP

#include "sv_sender.hpp"
#include "Ethernet.hpp"
#include "Goose.hpp"
#include "SampledValue.hpp"
#include "Virtual_LAN.hpp"
#include "transient.hpp"
#include "sniffer.hpp"
#include <atomic>
#include <array>

std::vector<Goose_info> get_goose_input_config(const std::string& config_path);
std::vector<std::unique_ptr<transient_config>> get_transient_test_config(const std::string& config_path);

struct Sv_packet{
    std::vector<uint8_t> base_pkt;
    std::vector<uint32_t> data_pos;
    std::vector<uint32_t> smpCnt_pos;
    uint8_t noAsdu;
    uint8_t noChannels;
    uint16_t smpRate;
};

class Tests_Class{
public:
    std::array<std::atomic<uint8_t>, 16> digital_input;
    std::vector<std::unique_ptr<transient_config>> transient_tests;
    RawSocket raw_socket;
    SnifferClass sniffer;

private:
    
    uint8_t priority = 80;

public:

    Tests_Class(){
        for(size_t i = 0; i < digital_input.size(); ++i) {
            digital_input[i].store(0, std::memory_order_relaxed);
        }
        sniffer.digitalInput = &digital_input;
    }

    int32_t is_running(){
        for (auto& conf: transient_tests){
            if (conf->running.load(std::memory_order_acquire)){
                return 1;
            }
        }
        return 0;
    }

    void start_transient_test(std::vector<std::unique_ptr<transient_config>>&& configs){

        if(sniffer.running.load(std::memory_order_acquire)){
            sniffer.stopThread();
        }
        std::vector<Goose_info> goInput = get_goose_input_config("files/goose_input_config.json");
        sniffer.startThread(goInput);

        struct sched_param param;
        param.sched_priority = this->priority;
        transient_tests.clear();

        for (size_t i=0; i<configs.size(); i++){
            transient_tests.push_back(std::move(configs[i]));
        }

        for (auto& conf: transient_tests){
            conf->socket = &this->raw_socket;
            conf->digital_input = &this->digital_input;
            int ret = pthread_create(&conf->thd, NULL, run_transient_test, static_cast<void*>(conf.get()));
            if (ret != 0) {
                throw std::runtime_error("Failed to create transient test thread: " + std::string(strerror(ret)));
            }
            conf->threadStarted = true;
            pthread_setschedparam(conf->thd, SCHED_FIFO, &param);
        }
    }

    void stop_transient_test(){
        for (auto& conf: transient_tests){
            conf->stop.store(true, std::memory_order_release);
        }
    }
    
    void join_transient_tests(){
        for (auto& conf: transient_tests){
            if (conf->threadStarted) {
                pthread_join(conf->thd, NULL);
                conf->threadStarted = false;
            }
        }
    }

};

Sv_packet get_sampledValue_pkt_info(SampledValue_Config& svConf);

#endif