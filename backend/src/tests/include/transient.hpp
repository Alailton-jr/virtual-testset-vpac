#ifndef TRANSIENT_HPP
#define TRANSIENT_HPP

#include <vector>
#include <inttypes.h>
#include <atomic>
#include <array>
#include "sv_sender.hpp"
#include <pthread.h>
#include "raw_socket_platform.hpp"
#include <string>

#include <nlohmann/json.hpp>
#include <utility>
#include <fstream>
#include <time.h>

using json = nlohmann::json;

struct transient_config{

    std::string fileName;
    uint8_t loop_flag;
    uint8_t interval_flag;
    double interval;
    uint64_t start_time;
    uint32_t timed_start;
    int32_t fileloaded;
    std::string error_msg;

    struct timespec time_started;
    struct timespec time_ended;
    double trip_time;

    std::vector<std::vector<uint8_t>> channelConfig;
    std::vector<double> scale;
    double file_data_fs;
    SampledValue_Config sv_config;

    RawSocket* socket;
    std::array<std::atomic<uint8_t>, 16>* digital_input;
    

    std::atomic<bool> stop;
    std::atomic<bool> running;
    std::atomic<bool> error;
    pthread_t thd;
    bool threadStarted;
    
    transient_config() : stop(false), running(false), error(false), threadStarted(false) {}
};

void* run_transient_test(void* arg);

#endif // TRANSIENT_HPP

