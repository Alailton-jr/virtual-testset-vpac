#ifndef IEC61850_TYPES_HPP
#define IEC61850_TYPES_HPP

#include <vector>
#include <string>
#include <optional>
#include <inttypes.h>
#include <cstring>



class UtcTime {
public:
    uint32_t seconds;
    uint32_t fraction;
    uint8_t defined;
public:
    UtcTime(){
        defined = 0;
    }

    UtcTime(uint32_t seconds, uint32_t fraction) : seconds(seconds) {
        this->fraction = static_cast<uint32_t>((static_cast<uint64_t>(fraction) * (1LL << 32)) / 1000000000LL);
        defined = 1;
    }
    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;
        
        for (int i = 3; i >= 0; --i) {
            encoded.push_back((seconds >> (i * 8)) & 0xFF);
        }
        for (int i = 3; i >= 0; --i) {
            encoded.push_back((fraction >> (i * 8)) & 0xFF);
        }
        return encoded;
    }
    static std::vector<uint8_t> staticGetEncoded(uint32_t seconds, uint32_t fraction) {
        fraction = static_cast<uint32_t>((static_cast<uint64_t>(fraction) * (1LL << 32)) / 1000000000LL);
        std::vector<uint8_t> encoded(8);
        encoded[0] = (seconds >> 24) & 0xFF;
        encoded[1] = (seconds >> 16) & 0xFF;
        encoded[2] = (seconds >> 8) & 0xFF;
        encoded[3] = seconds & 0xFF;
        encoded[4] = (fraction >> 24) & 0xFF;
        encoded[5] = (fraction >> 16) & 0xFF;
        encoded[6] = (fraction >> 8) & 0xFF;
        encoded[7] = fraction & 0xFF;
        return encoded;
    }
};

class FloatingPoint {
    std::string value;
public:
    FloatingPoint(const std::string& val) : value(val) {}
    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;
        encoded.insert(encoded.end(), value.begin(), value.end());
        return encoded;
    }
};

class TimeOfDay {
    std::string value;
public:
    TimeOfDay(const std::string& val) : value(val) {}
    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;
        encoded.insert(encoded.end(), value.begin(), value.end());
        return encoded;
    }
};

class Data {
public:
        enum class Type {
        Array,
        Structure,
        Boolean,
        BitString,
        Integer,
        Unsigned,
        FloatingPoint,
        Real,
        OctetString,
        VisibleString,
        BinaryTime,
        Bcd,
        BooleanArray,
        ObjId,
        MmsString,
        UtcTime
    };

    Type type;
    std::optional<std::vector<Data>> array;
    std::optional<std::vector<Data>> structure;
    std::optional<bool> boolean;
    std::optional<std::vector<uint8_t>> bitString;
    std::optional<int32_t> integer;
    std::optional<uint32_t> unsignedInt;
    std::optional<FloatingPoint> floatingPoint;
    std::optional<double> real;
    std::optional<std::string> octetString;
    std::optional<std::string> visibleString;
    std::optional<TimeOfDay> binaryTime;
    std::optional<int32_t> bcd;
    std::optional<std::vector<uint8_t>> booleanArray;
    std::optional<std::string> objId;
    std::optional<std::string> mmsString;
    std::optional<UtcTime> utcTime;

    Data(const Type& type){
        this->type = type;
        switch (this->type) {
            case Type::Array:
                array = std::vector<Data>();
                break;
            case Type::Structure:
                structure = std::vector<Data>();
                break;
            case Type::Boolean:
                boolean = false;
                break;
            case Type::BitString:
                bitString = std::vector<uint8_t>();
                break;
            case Type::Integer:
                integer = 0;
                break;
            case Type::Unsigned:
                unsignedInt = 0;
                break;
            case Type::FloatingPoint:
                floatingPoint = FloatingPoint("0.0");
                break;
            case Type::Real:
                real = 0.0;
                break;
            case Type::OctetString:
                octetString = "";
                break;
            case Type::VisibleString:
                visibleString = "";
                break;
            case Type::BinaryTime:
                binaryTime = TimeOfDay("00:00:00.000000");
                break;
            case Type::Bcd:
                bcd = 0;
                break;
            case Type::BooleanArray:
                booleanArray = std::vector<uint8_t>();
                break;
            case Type::ObjId:
                objId = "";
                break;
            case Type::MmsString:
                mmsString = "";
                break;
            case Type::UtcTime:
                utcTime = UtcTime(0, 0);
                break;
        }
    }

    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;

        switch (type) {
            case Type::Array:
                // Encode array
                if (!array.has_value()) break;
                encoded.push_back(0xA1);
                for (const auto& item : array.value()) {
                    std::vector<uint8_t> itemEncoded = item.getEncoded();
                    encoded.insert(encoded.end(), itemEncoded.begin(), itemEncoded.end());
                }
                break;
            case Type::Structure:
                // Encode structure
                if (!structure.has_value()) break;
                encoded.push_back(0xA2);
                for (const auto& item : structure.value()) {
                    std::vector<uint8_t> itemEncoded = item.getEncoded();
                    encoded.insert(encoded.end(), itemEncoded.begin(), itemEncoded.end());
                }
                break;
            case Type::Boolean:
                if (!boolean.has_value()) break;
                encoded.push_back(0x83);
                encoded.push_back(1);
                encoded.push_back(boolean.value() ? 0xFF : 0x00);
                break;
            case Type::BitString:
                if (!this->bitString.has_value()) break;
                encoded.push_back(0x84);
                encoded.push_back(static_cast<uint8_t>(this->bitString.value().size()));
                encoded.insert(encoded.end(), this->bitString.value().begin(), this->bitString.value().end());
                break;
            case Type::Integer:
                if (!this->integer.has_value()) break;
                encoded.push_back(0x85);
                encoded.push_back(4);
                encoded.push_back((this->integer.value() >> 24) & 0xFF);
                encoded.push_back((this->integer.value() >> 16) & 0xFF);
                encoded.push_back((this->integer.value() >> 8) & 0xFF);
                encoded.push_back(this->integer.value() & 0xFF);
                break;
            case Type::Unsigned:
                if (!this->unsignedInt.has_value()) break;
                encoded.push_back(0x86);
                encoded.push_back(4);
                encoded.push_back((this->unsignedInt.value() >> 24) & 0xFF);
                encoded.push_back((this->unsignedInt.value() >> 16) & 0xFF);
                encoded.push_back((this->unsignedInt.value() >> 8) & 0xFF);
                encoded.push_back(this->unsignedInt.value() & 0xFF);
                break;
            case Type::FloatingPoint:
                if (!this->floatingPoint.has_value()) break;
                encoded.push_back(0x87);
                {
                    std::vector<uint8_t> fpEncoded = this->floatingPoint.value().getEncoded();
                    encoded.push_back(static_cast<uint8_t>(fpEncoded.size()));
                    encoded.insert(encoded.end(), fpEncoded.begin(), fpEncoded.end());
                }
                break;
            case Type::Real:
                // IEC 61850-7-2: Real is encoded as 64-bit IEEE 754 double (8 bytes)
                if (!this->real.has_value()) break;
                encoded.push_back(0x88);
                encoded.push_back(8);  // Length: 8 bytes for double
                {
                    std::vector<uint8_t> realEncoded(8);  // Changed from 4 to 8
                    memcpy(realEncoded.data(), &this->real.value(), 8);  // Copy full 8 bytes
                    encoded.insert(encoded.end(), realEncoded.begin(), realEncoded.end());
                }
                break;
            case Type::OctetString:
                if (!this->octetString.has_value()) break;
                encoded.push_back(0x89);
                encoded.push_back(static_cast<uint8_t>(this->octetString.value().size()));
                encoded.insert(encoded.end(), this->octetString.value().begin(), this->octetString.value().end());
                break;
            case Type::VisibleString:
                if (!this->visibleString.has_value()) break;
                encoded.push_back(0x8A);
                encoded.push_back(static_cast<uint8_t>(this->visibleString.value().size()));
                encoded.insert(encoded.end(), this->visibleString.value().begin(), this->visibleString.value().end());
                break;
            case Type::BinaryTime:
                if (!this->binaryTime.has_value()) break;
                encoded.push_back(0x8B);
                {
                    std::vector<uint8_t> btEncoded = this->binaryTime.value().getEncoded();
                    encoded.push_back(static_cast<uint8_t>(btEncoded.size()));
                    encoded.insert(encoded.end(), btEncoded.begin(), btEncoded.end());
                }
                break;
            case Type::Bcd:
                if (!this->bcd.has_value()) break;
                encoded.push_back(0x8C);
                encoded.push_back(4);
                encoded.push_back((this->bcd.value() >> 24) & 0xFF);
                encoded.push_back((this->bcd.value() >> 16) & 0xFF);
                encoded.push_back((this->bcd.value() >> 8) & 0xFF);
                encoded.push_back(this->bcd.value() & 0xFF);
                break;
            case Type::BooleanArray:
                if (!this->booleanArray.has_value()) break;
                encoded.push_back(0x8D);
                encoded.push_back(static_cast<uint8_t>(this->booleanArray.value().size()));
                encoded.insert(encoded.end(), this->booleanArray.value().begin(), this->booleanArray.value().end());
                break;
            case Type::ObjId:
                if (!this->objId.has_value()) break;
                encoded.push_back(0x8E);
                encoded.push_back(static_cast<uint8_t>(this->objId.value().size()));
                encoded.insert(encoded.end(), this->objId.value().begin(), this->objId.value().end());
                break;
            case Type::MmsString:
                if (!this->mmsString.has_value()) break;
                encoded.push_back(0x8F);
                encoded.push_back(static_cast<uint8_t>(this->mmsString.value().size()));
                encoded.insert(encoded.end(), this->mmsString.value().begin(), this->mmsString.value().end());
                break;
            case Type::UtcTime:
                if (!this->utcTime.has_value()) break;
                encoded.push_back(0x90);
                {
                    std::vector<uint8_t> utcEncoded = this->utcTime.value().getEncoded();
                    encoded.push_back(static_cast<uint8_t>(utcEncoded.size()));
                    encoded.insert(encoded.end(), utcEncoded.begin(), utcEncoded.end());
                }
                break;
        }
        return encoded;
    }
};

#endif // IEC61850_TYPES_HPP