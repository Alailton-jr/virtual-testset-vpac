#ifndef GOOSE_HPP
#define GOOSE_HPP

#include <vector>
#include <string>
#include <optional>
#include <unordered_map>
#include <inttypes.h>
#include <cstring>
#include <stdexcept>

#include "IEC61850_Types.hpp"
#include "BER_Encoding.hpp"


class Goose {
public:
    uint16_t appID;
    uint16_t reserved1 = 0;
    uint16_t reserved2 = 0;

    std::string gocbRef;
    int32_t timeAllowedtoLive;
    std::string datSet;
    std::optional<std::string> goID;
    UtcTime t;
    int32_t stNum;
    int32_t sqNum;
    bool simulation = false;
    int32_t confRev;
    bool ndsCom = false;
    int32_t numDatSetEntries;
    std::vector<Data> allData;

    mutable std::vector<uint8_t> encoded;
    mutable std::unordered_map<std::string, size_t> indices;

    int offSet;

        Goose(const std::string& /*srcMAC*/, const std::string& /*dstMAC*/, uint16_t appID,
              uint16_t /*vlan_id*/, const std::string& gocbRef, uint32_t timeAllowedToLive,
              const std::string& datSet, const std::string& /*goID*/, UtcTime t, uint32_t stNum,
              uint32_t sqNum, bool /*test*/, uint32_t confRev, bool /*ndsCom*/, uint32_t /*numDatSetEntries*/,
              const std::vector<Data>& allData)
        : appID(appID), gocbRef(gocbRef), timeAllowedtoLive(static_cast<int32_t>(timeAllowedToLive)), datSet(datSet),
          t(t), stNum(static_cast<int32_t>(stNum)), sqNum(static_cast<int32_t>(sqNum)), 
          confRev(static_cast<int32_t>(confRev)), numDatSetEntries(0), allData(allData), offSet(0) {}

    int getParamPos(const std::string& param) const {
        auto it = indices.find(param);
        if (it != indices.end()) {
            return static_cast<int>(it->second + static_cast<size_t>(this->offSet));
        }
        return -1;
    }

    std::vector<uint8_t> getEncoded() {
        encoded.clear();  // Clear previous encoding
        encoded.reserve(2048);

        std::vector<uint8_t> pduEncoded = this->getPduEncoded();

        uint16_t pduSize = static_cast<uint16_t>(pduEncoded.size());
        uint16_t length = 8 + 2 + pduSize;
        offSet = 14;

    numDatSetEntries = static_cast<int32_t>(allData.size());

        if (pduSize > 0xff) {
            length += 2;
            offSet += 2;
        } else if (pduSize > 0x80) {
            length += 1;
            offSet += 1;
        }

        // EtherType
        encoded.push_back(0x88);
        encoded.push_back(0xb8);

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
        encoded.push_back(0x61);
        if (pduSize > 0xff) {
            encoded.push_back(0x82);
            encoded.push_back((pduSize >> 8) & 0xFF);
            encoded.push_back(pduSize & 0xFF);
        } else if (pduSize > 0x80) {
            encoded.push_back(0x81);
            encoded.push_back(pduSize & 0xFF);
        } else {
            encoded.push_back(pduSize & 0xFF);
        }
        encoded.insert(encoded.end(), pduEncoded.begin(), pduEncoded.end());

        return encoded;
    }

private:
    std::vector<uint8_t> getPduEncoded() {
        std::vector<uint8_t> _encoded;
        indices.clear(); 

    this->numDatSetEntries = static_cast<int32_t>(allData.size());
        
        // Guard against dataset overflow (32-bit signed integer max)
        if (this->numDatSetEntries > INT32_MAX) {
            throw std::runtime_error("numDatSetEntries exceeds maximum allowed value");
        }

        // gocbRef
        indices["gocbRef"] = _encoded.size();
        _encoded.push_back(0x80); // Tag [0] VisibleString
        auto gocbRefLen = encodeBERLength(gocbRef.size());
        _encoded.insert(_encoded.end(), gocbRefLen.begin(), gocbRefLen.end());
        _encoded.insert(_encoded.end(), gocbRef.begin(), gocbRef.end());

        // timeAllowedtoLive
        indices["timeAllowedtoLive"] = _encoded.size();
        _encoded.push_back(0x81); // Tag [1] INTEGER
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((timeAllowedtoLive >> (i * 8)) & 0xFF);
        }

        // datSet
        indices["datSet"] = _encoded.size();
        _encoded.push_back(0x82); // Tag [2] VisibleString
        auto datSetLen = encodeBERLength(datSet.size());
        _encoded.insert(_encoded.end(), datSetLen.begin(), datSetLen.end());
        _encoded.insert(_encoded.end(), datSet.begin(), datSet.end());

        // goID
        if (goID) {
            indices["goID"] = _encoded.size();
            _encoded.push_back(0x83); // Tag [3] VisibleString OPTIONAL
            auto goIDLen = encodeBERLength(goID->size());
            _encoded.insert(_encoded.end(), goIDLen.begin(), goIDLen.end());
            _encoded.insert(_encoded.end(), goID->begin(), goID->end());
        }

        // t
        indices["t"] = _encoded.size();
        _encoded.push_back(0x84); // Tag [4] UtcTime
        auto tEncoded = t.getEncoded();
        auto tLen = encodeBERLength(tEncoded.size());
        _encoded.insert(_encoded.end(), tLen.begin(), tLen.end());
        _encoded.insert(_encoded.end(), tEncoded.begin(), tEncoded.end());

        // stNum
        indices["stNum"] = _encoded.size();
        _encoded.push_back(0x85); // Tag [5] INTEGER (stNum)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((stNum >> (i * 8)) & 0xFF);
        }

        // sqNum
        indices["sqNum"] = _encoded.size();
        _encoded.push_back(0x86); // Tag [6] INTEGER (sqNum)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((sqNum >> (i * 8)) & 0xFF);
        }

        // Simulation
        indices["simulation"] = _encoded.size();
        _encoded.push_back(0x87); // Tag [7] BOOLEAN (simulation)
        _encoded.push_back(1);
        _encoded.push_back(simulation ? 0xFF : 0x00);

        // confRev
        indices["confRev"] = _encoded.size();
        _encoded.push_back(0x88); // Tag [8] INTEGER (confRev)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((confRev >> (i * 8)) & 0xFF);
        }


        // ndscom
        indices["ndsCom"] = _encoded.size();
        _encoded.push_back(0x89); // Tag [9] BOOLEAN (ndsCom)
        _encoded.push_back(1);
        _encoded.push_back(ndsCom ? 0xFF : 0x00);

        // numDatSetEntries
        indices["numDatSetEntries"] = _encoded.size();
        _encoded.push_back(0x8A); // Tag [10] INTEGER (numDatSetEntries)
        _encoded.push_back(4);
        for (int i = 3; i >= 0; --i) {
            _encoded.push_back((numDatSetEntries >> (i * 8)) & 0xFF);
        }

        // Encode allData
        indices["allData"] = _encoded.size();
        _encoded.push_back(0xab); // Tag [11] SEQUENCE OF Data
        std::vector<uint8_t> allDataEncoded;
        for (const auto& data : allData) {
            auto dataEncoded = data.getEncoded();
            allDataEncoded.insert(allDataEncoded.end(), dataEncoded.begin(), dataEncoded.end());
        }
        // Use BER encoding for length (handles >255 case)
        auto lengthBytes = encodeBERLength(allDataEncoded.size());
        _encoded.insert(_encoded.end(), lengthBytes.begin(), lengthBytes.end());
        _encoded.insert(_encoded.end(), allDataEncoded.begin(), allDataEncoded.end());

        return _encoded;
    }
};




#endif // GOOSE_HPP