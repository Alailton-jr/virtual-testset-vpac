
#ifndef SV_SENDER_HPP
#define SV_SENDER_HPP

#include <vector>
#include <string>
#include <inttypes.h>
#include "raw_socket_platform.hpp"
#include "general_definition.hpp"

class SampledValue_Config{
public:

    // Sender information
    uint16_t smpRate;
    uint16_t noChannels;
    RawSocket* raw_socket;

    // Ethernet PKT
    std::string srcMac = "01:0c:cd:04:00:01";
    std::string dstMac;
    // VLAN PKT
    uint16_t vlanId = 0x8100;
    uint8_t vlanPcp = 0;
    uint8_t vlanDei = 0;
    // Sampled Value PKT
    uint16_t appID;
    uint8_t noAsdu;
    std::string svID;
    uint16_t smpCnt;
    uint32_t confRev;
    uint8_t smpSynch;
    uint16_t smpMod;
public:
    SampledValue_Config(){
        // Phase 6: Use getInterfaceName() for environment variable override support
        this->dstMac = GetMACAddress(getInterfaceName().c_str());
    }
};



#endif
