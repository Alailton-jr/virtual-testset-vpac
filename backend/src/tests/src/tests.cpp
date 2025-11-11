

#include "tests.hpp"

#include <nlohmann/json.hpp>
#include <utility>
#include <stdexcept>
#include <string>
#include <cctype>
using json = nlohmann::json;

void from_json(const json& j, SampledValue_Config& sv) {
    // Phase 6: Strict validation with clear error messages
    
    // Required fields
    if (!j.contains("appID")) throw std::invalid_argument("SampledValue_Config: missing required field 'appID'");
    if (!j.contains("confRev")) throw std::invalid_argument("SampledValue_Config: missing required field 'confRev'");
    if (!j.contains("dstMac")) throw std::invalid_argument("SampledValue_Config: missing required field 'dstMac'");
    if (!j.contains("noAsdu")) throw std::invalid_argument("SampledValue_Config: missing required field 'noAsdu'");
    if (!j.contains("smpCnt")) throw std::invalid_argument("SampledValue_Config: missing required field 'smpCnt'");
    if (!j.contains("smpMod")) throw std::invalid_argument("SampledValue_Config: missing required field 'smpMod'");
    if (!j.contains("smpRate")) throw std::invalid_argument("SampledValue_Config: missing required field 'smpRate'");
    if (!j.contains("smpSynch")) throw std::invalid_argument("SampledValue_Config: missing required field 'smpSynch'");
    if (!j.contains("svID")) throw std::invalid_argument("SampledValue_Config: missing required field 'svID'");
    if (!j.contains("vlanDei")) throw std::invalid_argument("SampledValue_Config: missing required field 'vlanDei'");
    if (!j.contains("vlanId")) throw std::invalid_argument("SampledValue_Config: missing required field 'vlanId'");
    if (!j.contains("vlanPcp")) throw std::invalid_argument("SampledValue_Config: missing required field 'vlanPcp'");
    if (!j.contains("noChannels")) throw std::invalid_argument("SampledValue_Config: missing required field 'noChannels'");
    
    j.at("appID").get_to(sv.appID);
    j.at("confRev").get_to(sv.confRev);
    j.at("dstMac").get_to(sv.dstMac);
    j.at("noAsdu").get_to(sv.noAsdu);
    j.at("smpCnt").get_to(sv.smpCnt);
    j.at("smpMod").get_to(sv.smpMod);
    j.at("smpRate").get_to(sv.smpRate);
    j.at("smpSynch").get_to(sv.smpSynch);
    j.at("svID").get_to(sv.svID);
    j.at("vlanDei").get_to(sv.vlanDei);
    j.at("vlanId").get_to(sv.vlanId);
    j.at("vlanPcp").get_to(sv.vlanPcp);
    j.at("noChannels").get_to(sv.noChannels);
    
    // Range validation
    if (sv.smpRate == 0) {
        throw std::invalid_argument("SampledValue_Config: smpRate must be > 0");
    }
    if (sv.noChannels > 32) {
        throw std::invalid_argument("SampledValue_Config: noChannels must be <= 32 (got " + std::to_string(sv.noChannels) + ")");
    }
    
    // MAC address format validation (XX:XX:XX:XX:XX:XX)
    if (sv.dstMac.length() != 17) {
        throw std::invalid_argument("SampledValue_Config: dstMac must be 17 characters (XX:XX:XX:XX:XX:XX format)");
    }
    for (size_t i = 0; i < sv.dstMac.length(); ++i) {
        if (i % 3 == 2) {
            if (sv.dstMac[i] != ':') {
                throw std::invalid_argument("SampledValue_Config: dstMac format invalid (expected ':' at position " + std::to_string(i) + ")");
            }
        } else {
            if (!std::isxdigit(static_cast<unsigned char>(sv.dstMac[i]))) {
                throw std::invalid_argument("SampledValue_Config: dstMac contains non-hex character at position " + std::to_string(i));
            }
        }
    }
}

void from_json(const json& j, transient_config& cfg) {
    // Phase 6: Strict validation
    if (!j.contains("channelConfig")) throw std::invalid_argument("transient_config: missing required field 'channelConfig'");
    if (!j.contains("file_data_fs")) throw std::invalid_argument("transient_config: missing required field 'file_data_fs'");
    if (!j.contains("fileName")) throw std::invalid_argument("transient_config: missing required field 'fileName'");
    if (!j.contains("interval")) throw std::invalid_argument("transient_config: missing required field 'interval'");
    if (!j.contains("interval_flag")) throw std::invalid_argument("transient_config: missing required field 'interval_flag'");
    if (!j.contains("loop_flag")) throw std::invalid_argument("transient_config: missing required field 'loop_flag'");
    if (!j.contains("scale")) throw std::invalid_argument("transient_config: missing required field 'scale'");
    if (!j.contains("timed_start")) throw std::invalid_argument("transient_config: missing required field 'timed_start'");
    if (!j.contains("start_time")) throw std::invalid_argument("transient_config: missing required field 'start_time'");
    if (!j.contains("sv_config")) throw std::invalid_argument("transient_config: missing required field 'sv_config'");
    
    j.at("channelConfig").get_to(cfg.channelConfig);
    j.at("file_data_fs").get_to(cfg.file_data_fs);
    j.at("fileName").get_to(cfg.fileName);
    j.at("interval").get_to(cfg.interval);
    j.at("interval_flag").get_to(cfg.interval_flag);
    j.at("loop_flag").get_to(cfg.loop_flag);
    j.at("scale").get_to(cfg.scale);
    j.at("timed_start").get_to(cfg.timed_start);
    j.at("start_time").get_to(cfg.start_time);
    j.at("sv_config").get_to(cfg.sv_config);
    
    // Range validation
    if (cfg.file_data_fs == 0) {
        throw std::invalid_argument("transient_config: file_data_fs must be > 0");
    }
    // Note: scale is a vector, would need element-wise validation if needed
}

void from_json(const json& j, Goose_info& cfg) {
    // Phase 6: Strict validation
    if (!j.contains("goCbRef")) throw std::invalid_argument("Goose_info: missing required field 'goCbRef'");
    if (!j.contains("mac_dst")) throw std::invalid_argument("Goose_info: missing required field 'mac_dst'");
    if (!j.contains("input")) throw std::invalid_argument("Goose_info: missing required field 'input'");
    
    j.at("goCbRef").get_to(cfg.goCbRef);
    j.at("mac_dst").get_to(cfg.mac_dst);
    j.at("input").get_to(cfg.input);
    
    // MAC address validation - expecting vector of 6 bytes
    if (cfg.mac_dst.size() != 6) {
        throw std::invalid_argument("Goose_info: mac_dst must be 6 bytes");
    }
    // Note: Further MAC address validation could be added here if needed
}

std::vector<Goose_info> get_goose_input_config(const std::string& config_path){
    std::vector<Goose_info> goInputs;
    std::ifstream f(config_path);
    if (!f.is_open()) {
        return {};
    }
    try{
        json data = json::parse(f);
        const auto& test_configs = data.at("Goose_info");
        for (const auto& test_entry : test_configs) {
            Goose_info cfg = test_entry.get<Goose_info>();
            goInputs.push_back(cfg);
        }
    } catch (const json::exception& e) {
        return {};
    }
    return goInputs;
}


std::vector<std::unique_ptr<transient_config>> get_transient_test_config(const std::string& config_path) {
    std::vector<std::unique_ptr<transient_config>> transient_configs;
    std::ifstream f(config_path);
    if (!f.is_open()) {
        auto test_config = std::make_unique<transient_config>();
        test_config->error_msg = "Failed to open config file: " + config_path;
        test_config->fileloaded = 0;
        transient_configs.push_back(std::move(test_config));
        return transient_configs;
    }
    try {
        json data = json::parse(f);
        const auto& test_configs = data.at("Test Config");
        for (const auto& test_entry : test_configs) {
            if (test_entry.at("test_type").get<std::string>() == "transient") {
                auto cfg = std::make_unique<transient_config>();
                // Manually extract fields from JSON to avoid copying
                cfg->fileName = "files/" + test_entry.at("fileName").get<std::string>();
                cfg->loop_flag = test_entry.value("loop_flag", uint8_t(0));
                cfg->interval_flag = test_entry.value("interval_flag", uint8_t(0));
                cfg->interval = test_entry.value("interval", 0.0);
                cfg->start_time = test_entry.value("start_time", uint64_t(0));
                cfg->timed_start = test_entry.value("timed_start", uint32_t(0));
                cfg->fileloaded = 1;
                
                // Extract channel config and scale if present
                if (test_entry.contains("channelConfig")) {
                    cfg->channelConfig = test_entry.at("channelConfig").get<std::vector<std::vector<uint8_t>>>();
                }
                if (test_entry.contains("scale")) {
                    cfg->scale = test_entry.at("scale").get<std::vector<double>>();
                }
                cfg->file_data_fs = test_entry.value("file_data_fs", 0.0);
                
                // Extract SV config if present
                if (test_entry.contains("sv_config")) {
                    const auto& sv = test_entry.at("sv_config");
                    cfg->sv_config.srcMac = sv.value("srcMac", "");
                    cfg->sv_config.dstMac = sv.value("dstMac", "");
                    cfg->sv_config.appID = sv.value("appID", uint16_t(0));
                    cfg->sv_config.vlanId = sv.value("vlanId", uint16_t(0));
                    cfg->sv_config.vlanPcp = static_cast<uint8_t>(sv.value("vlanPcp", uint16_t(0)));
                    cfg->sv_config.vlanDei = static_cast<uint8_t>(sv.value("vlanDei", uint16_t(0)));
                    cfg->sv_config.noAsdu = sv.value("noAsdu", uint8_t(0));
                    cfg->sv_config.svID = sv.value("svID", "");
                    cfg->sv_config.smpCnt = sv.value("smpCnt", uint16_t(0));
                    cfg->sv_config.confRev = sv.value("confRev", uint32_t(0));
                    cfg->sv_config.smpSynch = sv.value("smpSynch", uint8_t(0));
                    cfg->sv_config.smpRate = sv.value("smpRate", uint16_t(0));
                    cfg->sv_config.smpMod = sv.value("smpMod", uint16_t(0));
                    cfg->sv_config.noChannels = sv.value("noChannels", uint16_t(0));
                }
                
                transient_configs.push_back(std::move(cfg));
            }
        }
    } catch (const json::exception& e) {
        auto error_cfg = std::make_unique<transient_config>();
        error_cfg->error_msg = "JSON error: " + std::string(e.what());
        error_cfg->fileloaded = 0;
        transient_configs.push_back(std::move(error_cfg));
    }
    return transient_configs;
}

// Pre-build SV packet template with static fields
// Only dynamic fields (smpCnt, seqData) need patching per frame
// PERF: Call once per config and cache result; avoid per-frame allocation
Sv_packet get_sampledValue_pkt_info(SampledValue_Config& svConf){

    Sv_packet packetInfo;

    // 
    packetInfo.noAsdu = svConf.noAsdu;
    packetInfo.smpRate = svConf.smpRate;
    packetInfo.noChannels = static_cast<uint8_t>(svConf.noChannels);

    // Ethernet (static: source/dest MAC)
    Ethernet eth(svConf.srcMac, svConf.dstMac);
    auto encoded_eth = eth.getEncoded();
    packetInfo.base_pkt.reserve(encoded_eth.size() + 200);  // Pre-allocate typical size
    packetInfo.base_pkt.insert(packetInfo.base_pkt.end(), encoded_eth.begin(), encoded_eth.end());

    // Virtual LAN (static: priority, DEI, ID)
    Virtual_LAN vlan(svConf.vlanPcp, svConf.vlanDei, svConf.vlanId);
    auto encoded_vlan = vlan.getEncoded();
    packetInfo.base_pkt.insert(packetInfo.base_pkt.end(), encoded_vlan.begin(), encoded_vlan.end());

    // SampledValue (mostly static: appID, svID, confRev, smpSynch, smpMod)
    SampledValue sv(
        svConf.appID,
        svConf.noAsdu,
        svConf.svID,
        svConf.smpCnt,
        svConf.confRev,
        svConf.smpSynch,
        svConf.smpMod
    );

    // Initial position of SampledValue block
    size_t idx_SV_Start = packetInfo.base_pkt.size();

    auto encoded_sv = sv.getEncoded(8);
    packetInfo.base_pkt.insert(packetInfo.base_pkt.end(), encoded_sv.begin(), encoded_sv.end());

    // Record positions of dynamic fields for fast patching
    packetInfo.data_pos.reserve(svConf.noAsdu);
    packetInfo.smpCnt_pos.reserve(svConf.noAsdu);
    
    for (int num=0; num<svConf.noAsdu; num++){
        int data_pos = sv.getParamPos(num, "seqData");
        int smpCont_pos = sv.getParamPos(num, "smpCnt");
        
        if (data_pos >= 0) {
            packetInfo.data_pos.push_back(static_cast<uint32_t>(data_pos) + static_cast<uint32_t>(idx_SV_Start));
        }
        if (smpCont_pos >= 0) {
            packetInfo.smpCnt_pos.push_back(static_cast<uint32_t>(smpCont_pos) + static_cast<uint32_t>(idx_SV_Start));
        }
    }

    return packetInfo;
}

