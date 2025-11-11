
#include "transient.hpp"

#include "raw_socket_platform.hpp"  // Use platform-aware selector instead of raw_socket.hpp
#include "signal_processing.hpp"
#include "timers.hpp"
#include "tests.hpp"
#include "rt_utils.hpp"
#include "logger.hpp"
#include "metrics.hpp"
#include <time.h>

#include <fstream>
#include <sstream>

struct transient_plan{
    void (*_execute)(transient_plan* plan);
    void execute(){
        _execute(this);
    }

    std::vector<std::vector<int32_t>>* buffer;
    Sv_packet* sv_info;
    RawSocket* socket;

    uint8_t loop_flag;
    uint8_t interval_flag;

    struct timespec start_time;
    int32_t timedStart;

    double interval;
    std::atomic<bool>* stop;
    std::array<std::atomic<uint8_t>, 16>* digital_input;

    struct timespec real_time_started, real_time_ended;
    double time_started, time_ended;
};

std::vector<std::vector<double>> getDataFromCsv(const std::string path){

    std::vector<std::vector<double>> data;

    std::ifstream file(path);

    if (!file.is_open()) {
        return {};
    }

    std::string line;
    std::getline(file, line);
    while (std::getline(file, line)){
        std::vector<double> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')){
            row.push_back(std::stod(cell));
        }
        data.push_back(row);
    }

    std::vector<std::vector<double>> transposed_data(data[0].size(), std::vector<double>(data.size()));
    for (size_t i = 0; i < data.size(); ++i) {
        for (size_t j = 0; j < data[i].size(); ++j) {
            transposed_data[j][i] = data[i][j];
        }
    }
    return transposed_data;
}

std::vector<std::vector<int32_t>> getTransientData(transient_config* conf){
    
    if (conf->fileName.empty()){
        conf->error.store(true, std::memory_order_release);
    }

    std::vector<std::vector<double>> data = getDataFromCsv(conf->fileName);
    if (data.size() < 1) {
        LOG_ERROR("TEST", "Transient test data file not found: %s", conf->fileName.c_str());
        return{};
    }

    data = resample(data, static_cast<float>(conf->file_data_fs), static_cast<float>(conf->sv_config.smpRate));

    std::vector<std::vector<int32_t>> res;
    res.resize(conf->sv_config.noChannels);

    for (auto& pos : conf->channelConfig){
        // pos[0] -> Channel
        // pos[1] -> csv data column
        int n_channel = pos[0];
        int n_data = pos[1];

        std::vector<int32_t> channel_data;
        channel_data.reserve(data[static_cast<size_t>(n_data)].size());  // Pre-allocate
        
        for (size_t j = 0; j < data[static_cast<size_t>(n_data)].size(); j++){
            channel_data.push_back(static_cast<int32_t>(data[static_cast<size_t>(n_data)][j] * conf->scale[static_cast<size_t>(n_channel)]));
        }
        res[static_cast<size_t>(n_channel)] = std::move(channel_data);  // Move to avoid copy
    }

    return res;
}

int updatePkt(std::vector<std::vector<int32_t>>* buffer, Sv_packet* pkt_info, int& idx, int& smpCount){

    int restartbuffer = 0;
    for (int num = 0; num < pkt_info->noAsdu; num++){

        // Ensure smpCount is within 16-bit range
        uint16_t safe_smpCount = static_cast<uint16_t>(smpCount);
        pkt_info->base_pkt[pkt_info->smpCnt_pos[static_cast<size_t>(num)]] = (safe_smpCount >> 8) & 0xFF;
        pkt_info->base_pkt[pkt_info->smpCnt_pos[static_cast<size_t>(num)]+1] = safe_smpCount & 0xFF;

        for (int cn = 0; cn < pkt_info->noChannels; cn++){
            if ((*buffer)[static_cast<size_t>(cn)].empty()) continue;
            size_t data_base = pkt_info->data_pos[static_cast<size_t>(num)];
            size_t cn_offset = static_cast<size_t>(cn) * 8;
            int32_t value = (*buffer)[static_cast<size_t>(cn)][static_cast<size_t>(idx)];
            pkt_info->base_pkt[data_base + cn_offset] = static_cast<uint8_t>((value >> 24) & 0xFF);
            pkt_info->base_pkt[data_base + cn_offset + 1] = static_cast<uint8_t>((value >> 16) & 0xFF);
            pkt_info->base_pkt[data_base + cn_offset + 2] = static_cast<uint8_t>((value >> 8) & 0xFF);
            pkt_info->base_pkt[data_base + cn_offset + 3] = static_cast<uint8_t>(value & 0xFF);
            restartbuffer = -cn;
        }
        idx = idx + 1;
        smpCount = smpCount + 1;

        // Wrap at sample rate (typically 4800) or enforce 16-bit wrap at 65536
        if (smpCount >= pkt_info->smpRate){
            smpCount = 0;
        }
        if (restartbuffer < 0 && static_cast<size_t>(idx) >= (*buffer)[static_cast<size_t>(-restartbuffer)].size()){
            idx = 0;
            restartbuffer = 1;
        }
    }
    return restartbuffer > 0;
}

void simple_replay(transient_plan* plan){

    Timer timer;
    struct timespec t_ini, t_end, t0, t1;

    long waitPeriod = static_cast<long>(1e9/plan->sv_info->smpRate);

    clock_gettime(CLOCK_MONOTONIC, &t_ini);

    if (!plan->timedStart){
        if (t_ini.tv_nsec > static_cast<long>(5e8)){
            t_ini.tv_sec += 2;
        }else{
            t_ini.tv_sec += 1;
        }
        t_ini.tv_nsec = 0;
    }else{
        if (t_ini.tv_sec < plan->start_time.tv_sec){
            t_ini.tv_sec = plan->start_time.tv_sec;
            t_ini.tv_nsec = plan->start_time.tv_nsec;
        }
    }

    int buffer_idx = 0;
    int smpCount = 0;
    ssize_t sizeSented = 0;
    long nPkts = 0;
    
    updatePkt(plan->buffer, plan->sv_info, buffer_idx, smpCount);
    timer.start_period(t_ini);
    timer.wait_period(waitPeriod);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while ((!plan->stop->load(std::memory_order_acquire)) && ((*plan->digital_input)[0].load(std::memory_order_acquire) == 0)){
#ifdef __linux__
        sizeSented = sendmsg(plan->socket->socket_id, &plan->socket->msg_hdr, 0);
#else
        // macOS: Raw sockets not supported, skip packet sending
        sizeSented = 0;
        (void)plan; // Suppress unused warning
#endif
        if (sizeSented > 0) {
            METRIC_SENT_FRAME();
        }
        if (updatePkt(plan->buffer, plan->sv_info, buffer_idx, smpCount)){
            break;
        }
        (void)nPkts; // Track packets sent (currently unused)
        nPkts++;
        timer.wait_period(waitPeriod);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    clock_gettime(CLOCK_MONOTONIC, &t_end);
    plan->real_time_started = t_ini;
    plan->real_time_ended = t_end;
    plan->time_started = t0.tv_sec + t0.tv_nsec * 1e-9;
    plan->time_ended = t1.tv_sec + t1.tv_nsec * 1e-9;
    return;
}

void loop_replay(transient_plan* plan){

    Timer timer;
    struct timespec t_ini, t_end, t0, t1;

    long waitPeriod = static_cast<long>(1e9/plan->sv_info->smpRate);

    clock_gettime(CLOCK_REALTIME, &t_end);
    clock_gettime(CLOCK_MONOTONIC, &t_ini);

    if (t_end.tv_nsec > static_cast<long>(5e8)){
        t_ini.tv_sec += 2;
    }else{
        t_ini.tv_sec += 1;
    }
    t_ini.tv_nsec = (t_ini.tv_nsec - t_end.tv_nsec);
    if (t_ini.tv_nsec < 0) t_ini.tv_nsec = static_cast<long>(1e9) - t_ini.tv_nsec;

    int buffer_idx = 0;
    int smpCount = 0;
    ssize_t sizeSented = 0;

    int n_stop = 0;
    (void)n_stop; // Currently unused

    updatePkt(plan->buffer, plan->sv_info, buffer_idx, smpCount);
    timer.start_period(t_ini);
    timer.wait_period(waitPeriod);
    clock_gettime(CLOCK_MONOTONIC, &t0);
    while ((!plan->stop->load(std::memory_order_acquire)) && ((*plan->digital_input)[0].load(std::memory_order_acquire) == 0)){
#ifdef __linux__
        sizeSented = sendmsg(plan->socket->socket_id, &plan->socket->msg_hdr, 0);
#else
        sizeSented = 0;
        (void)plan;
#endif
        if (sizeSented > 0) {
            METRIC_SENT_FRAME();
        }
        updatePkt(plan->buffer, plan->sv_info, buffer_idx, smpCount);
        timer.wait_period(waitPeriod);
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    plan->time_started = t0.tv_sec + t0.tv_nsec * 1e-9;
    plan->time_ended = t1.tv_sec + t1.tv_nsec * 1e-9;

    return;
}

void interval_replay(transient_plan* plan){
    (void)plan; // Not yet implemented
}


transient_plan create_plan(transient_config* conf, std::vector<std::vector<int32_t>>* data, Sv_packet* sv_info, RawSocket *socket){
    
    transient_plan plan;
    plan.buffer = data;
    plan.loop_flag = conf->loop_flag;
    plan.interval_flag = conf->interval_flag;
    plan.interval = conf->interval;
    plan.stop = &conf->stop;
    plan.sv_info = sv_info;
    plan.socket = socket;
    // digital_input is already a pointer in transient_config, so just assign it
    plan.digital_input = conf->digital_input;

    plan.timedStart = static_cast<int32_t>(conf->timed_start);
    plan.start_time.tv_sec = static_cast<time_t>(conf->start_time / 1e9);
    plan.start_time.tv_nsec = static_cast<long>(conf->start_time - static_cast<double>(plan.start_time.tv_sec) * 1e9);

    if (plan.loop_flag){
        plan._execute = &loop_replay;
    } else if (plan.interval_flag){
        plan._execute = &interval_replay;
    } else {
        plan._execute = &simple_replay;
    }

    return plan;
}


void* run_transient_test(void* arg){

    auto conf = reinterpret_cast<transient_config*> (arg);
    
    // Phase 7: Real-time setup for critical transient test thread
    LOG_INFO("TEST", "[RT] Transient test thread starting with real-time capabilities...");
    
    // Set real-time priority (slightly lower than sniffer for protection logic)
    rt_set_realtime(Protection_ThreadPriority);  // Default: 90 (configured in general_definition.hpp)
    
    // Optional: Set CPU affinity to isolate transient thread
    // Example: bind to CPU 4 for dedicated protection processing
    // rt_set_affinity({4});
    
    conf->running.store(true, std::memory_order_release);

    //Only for test - initialize digital input
    (*conf->digital_input)[0].store(0, std::memory_order_relaxed);

    std::vector<std::vector<int32_t>> buffer = getTransientData(conf);
    if (buffer.empty()){
        conf->running.store(false, std::memory_order_release);
        return nullptr;
    }
    Sv_packet sv_info = get_sampledValue_pkt_info(conf->sv_config);
    transient_plan plan = create_plan(conf, &buffer, &sv_info, conf->socket);

    plan.socket->iov.iov_base = (void*)sv_info.base_pkt.data();
    plan.socket->iov.iov_len = sv_info.base_pkt.size();

    try{
        plan.execute(); 
    }catch(...){}

    // std::cout << "" << plan.time_ended - plan.time_started << std::endl;

    conf->trip_time = plan.time_ended - plan.time_started;
    conf->time_started = plan.real_time_started;
    conf->time_ended = plan.real_time_started;

    conf->running.store(false, std::memory_order_release);
    return nullptr;
}







