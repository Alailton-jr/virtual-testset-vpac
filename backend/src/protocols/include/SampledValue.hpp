#ifndef SAMPLEDVALUE_HPP
#define SAMPLEDVALUE_HPP

#include <vector>
#include <string>
#include <unordered_map>
#include <inttypes.h>
#include <cstring>
#include "IEC61850_Types.hpp"


class SampledValue {
public:
    // Header
    uint16_t appID;
    uint16_t reserved1 = 0;
    uint16_t reserved2 = 0;

    // SAVPDU 0x60
    uint8_t noAsdu;             //0x80
    uint8_t security;           //0x81 - OPTIONAL

    //ASDU 0x30
    std::string svID;   //0x80
    std::string datSet; //0x81 - OPTIONAL
    uint16_t smpCnt;    //0x82
    uint32_t confRev;   //0x83
    UtcTime refrTm;     //0x84 - OPTIONAL
    uint8_t smpSynch;   //0x85
    uint16_t smpRate;   //0x86 - OPTIONAL
    // std::vector<std::vector<uint32_t>> seqData; //0x87
    uint16_t smpMod;     //0x88 - OPTIONAL

    mutable std::vector<uint8_t> encoded;
    mutable std::vector<std::unordered_map<std::string, size_t>> indices;
    int offSet;
    int asduSize;

    SampledValue(uint16_t appID, uint8_t noAsdu, const std::string &svID, uint16_t smpCnt, uint32_t confRev, uint8_t smpSynch, uint16_t smpMod)
        : appID(appID), noAsdu(noAsdu), svID(svID), smpCnt(smpCnt), confRev(confRev), smpSynch(smpSynch), smpMod(smpMod), offSet(0), asduSize(0) {
        
        this->indices.resize(noAsdu);
    }

    int getParamPos(int asduIndex, const std::string& param) const {
        // Bounds check: validate asduIndex is within valid range
        if (asduIndex < 0 || static_cast<size_t>(asduIndex) >= indices.size()) {
            return -1;  // Return error value for out-of-bounds access
        }
        auto it = indices[static_cast<size_t>(asduIndex)].find(param);
        if (it != indices[static_cast<size_t>(asduIndex)].end()) {
            size_t offset = it->second + static_cast<size_t>(this->offSet) + 
                           static_cast<size_t>(asduIndex) * static_cast<size_t>(this->asduSize);
            return static_cast<int>(offset);
        }
        return -1;
    }

    std::vector<uint8_t> getEncoded(uint8_t noChannel) {
        encoded.clear();  // Clear previous encoding
        encoded.reserve(2048);
        this->offSet = 0;

        std::vector<uint8_t> savPduEncoded = this->getSavPduEncoded(noChannel);

        uint16_t savPduSize = static_cast<uint16_t>(savPduEncoded.size());
        uint16_t length = 8 + 2 + savPduSize;
        this->offSet = 9;

        if (savPduSize > 0xff) {
            length += 2;
            offSet += 2;
        } else if (savPduSize > 0x80) {
            length += 1;
            offSet += 1;
        }

        // EtherType
        encoded.push_back(0x88);
        encoded.push_back(0xba);

        // APPID
        encoded.push_back((appID >> 8) & 0xFF);
        encoded.push_back(appID & 0xFF);

        // Length 
        encoded.push_back((length >> 8) & 0xFF);
        encoded.push_back(length & 0xFF);

        // Reserved 1
        encoded.push_back((reserved1 >> 8) & 0xFF);
        encoded.push_back(reserved1 & 0xFF);

        // Reserved 2
        encoded.push_back((reserved2 >> 8) & 0xFF);
        encoded.push_back(reserved2 & 0xFF);

        // PDU
        encoded.push_back(0x60);
        if (savPduSize > 0xff) {
            encoded.push_back(0x82);
            encoded.push_back((savPduSize >> 8) & 0xFF);
            encoded.push_back(savPduSize & 0xFF);
        } else if (savPduSize > 0x80) {
            encoded.push_back(0x81);
            encoded.push_back(savPduSize & 0xFF);
        } else {
            encoded.push_back(savPduSize & 0xFF);
        }
        offSet += encoded.size();
        encoded.insert(encoded.end(), savPduEncoded.begin(), savPduEncoded.end());

        return encoded;
    }

private:

    std::vector<uint8_t> getAsduEncoded(uint8_t noChannel, int asdu) {
        std::vector<uint8_t> _encoded;

        // svID
        indices[static_cast<size_t>(asdu)]["svID"] = _encoded.size();
        _encoded.push_back(0x80); // Tag [0] VisibleString
        _encoded.push_back(static_cast<uint8_t>(svID.size()));
        _encoded.insert(_encoded.end(), svID.begin(), svID.end());

        // datSet
        if (!datSet.empty()) {
            indices[static_cast<size_t>(asdu)]["datSet"] = _encoded.size();
            _encoded.push_back(0x81); // Tag [1] VisibleString
            _encoded.push_back(static_cast<uint8_t>(datSet.size()));
            _encoded.insert(_encoded.end(), datSet.begin(), datSet.end());
        }

        // smpCnt
        indices[static_cast<size_t>(asdu)]["smpCnt"] = _encoded.size();
        _encoded.push_back(0x82); // Tag [2] INTEGER
        _encoded.push_back(2);
        _encoded.push_back((smpCnt >> 8) & 0xFF);
        _encoded.push_back(smpCnt & 0xFF);

        // confRev
        indices[static_cast<size_t>(asdu)]["confRev"] = _encoded.size();
        _encoded.push_back(0x83); // Tag [3] INTEGER
        _encoded.push_back(4);
        _encoded.push_back((confRev >> 24) & 0xFF);
        _encoded.push_back((confRev >> 16) & 0xFF);
        _encoded.push_back((confRev >> 8) & 0xFF);
        _encoded.push_back(confRev & 0xFF);

        // refrTm
        if (refrTm.defined) {
            indices[static_cast<size_t>(asdu)]["refrTm"] = _encoded.size();
            _encoded.push_back(0x84); // Tag [4] UtcTime
            auto refrTmEncoded = refrTm.getEncoded();
            _encoded.push_back(static_cast<uint8_t>(refrTmEncoded.size()));
            _encoded.insert(_encoded.end(), refrTmEncoded.begin(), refrTmEncoded.end());
        }

        // smpSynch
        indices[static_cast<size_t>(asdu)]["smpSynch"] = _encoded.size();
        _encoded.push_back(0x85); // Tag [5] BOOLEAN
        _encoded.push_back(1);
        _encoded.push_back(smpSynch);

        // smpRate
        if (smpRate) {
            indices[static_cast<size_t>(asdu)]["smpRate"] = _encoded.size();
            _encoded.push_back(0x86); // Tag [6] INTEGER
            _encoded.push_back(2);
            _encoded.push_back((smpRate >> 8) & 0xFF);
            _encoded.push_back(smpRate & 0xFF);
        }

        // seqData
        indices[static_cast<size_t>(asdu)]["seqData"] = _encoded.size();
        _encoded.push_back(0x87); // Tag [7] SEQUENCE OF Data
        _encoded.push_back(noChannel*8);
        for (int channel =0;channel<noChannel;channel++){
            // For each channel, add 8 bytes of data
            for (int i = 3; i >= 0; --i) {
                _encoded.push_back(0);
                _encoded.push_back(0);
            }
        }

        // smpMod
        if (smpMod) {
            indices[static_cast<size_t>(asdu)]["smpMod"] = _encoded.size();
            _encoded.push_back(0x88); // Tag [8] INTEGER
            _encoded.push_back(2);
            _encoded.push_back((smpMod >> 8) & 0xFF);
            _encoded.push_back(smpMod & 0xFF);
        }

        return _encoded;
    }

    std::vector<uint8_t> getSeqAsduEncoded(uint8_t noChannel){
        std::vector<uint8_t> _encoded;

        for (int asdu = 0; asdu < this->noAsdu; asdu++){
            std::vector<uint8_t> asduEncoded = this->getAsduEncoded(noChannel, asdu);
            uint32_t asduSizeLocal = static_cast<uint32_t>(asduEncoded.size());

            _encoded.push_back(0x30); // Tag [0] SEQUENCE OF ASDU
            if (asduSizeLocal > 0xff) {
                _encoded.push_back(0x82);
                _encoded.push_back((asduSizeLocal >> 8) & 0xFF);
                _encoded.push_back(asduSizeLocal & 0xFF);
            } else if (asduSizeLocal > 0x80) {
                _encoded.push_back(0x81);
                _encoded.push_back(asduSizeLocal & 0xFF);
            } else {
                _encoded.push_back(asduSizeLocal & 0xFF);
            }
            _encoded.insert(_encoded.end(), asduEncoded.begin(), asduEncoded.end());

            if (asdu == 0){
                this->asduSize = static_cast<int>(_encoded.size());
            }
        }

        return _encoded;
    }

    std::vector<uint8_t> getSavPduEncoded(uint8_t noChannel) {
        std::vector<uint8_t> _encoded;
        indices.clear();

        // SEQ ASDU
        std::vector<uint8_t> seqAsduEncoded = this->getSeqAsduEncoded(noChannel);
    uint32_t seqAsduSize = static_cast<uint32_t>(seqAsduEncoded.size());

        // noAsdu
        _encoded.push_back(0x80); // Tag [0] INTEGER
        _encoded.push_back(1);
        _encoded.push_back(noAsdu);

        // security
        if (security) {
            _encoded.push_back(0x81); // Tag [1] BOOLEAN
            _encoded.push_back(1);
            _encoded.push_back(security ? 0xFF : 0x00);
        }

        this->offSet += _encoded.size();

        //seqAsdu
        _encoded.push_back(0xA2); // Tag [2] SEQUENCE OF ASDU
        if (seqAsduSize > 0xff) {
            _encoded.push_back(0x82);
            _encoded.push_back((seqAsduSize >> 8) & 0xFF);
            _encoded.push_back(seqAsduSize & 0xFF);
        } else if (seqAsduSize > 0x80) {
            _encoded.push_back(0x81);
            _encoded.push_back(seqAsduSize & 0xFF);
        } else {
            _encoded.push_back(seqAsduSize & 0xFF);
        }
        _encoded.insert(_encoded.end(), seqAsduEncoded.begin(), seqAsduEncoded.end());

        return _encoded;
    }
};







#endif // SAMPLEDVALUE_HPP



