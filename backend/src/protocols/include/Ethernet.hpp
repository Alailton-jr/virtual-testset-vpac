#ifndef ETHERNET_HPP
#define ETHERNET_HPP

#include <array>
#include <vector>
#include <string>
#include <inttypes.h>
#include <stdexcept>


class Ethernet {
public:
    std::string macSrc;
    std::string macDst;

    Ethernet(const std::string& dst, const std::string& src) : macSrc(src), macDst(dst) {
        // Validate MAC addresses at construction
        macStrToBytes(macSrc);
        macStrToBytes(macDst);
    }

    // Convert MAC string "XX:XX:XX:XX:XX:XX" to 6-byte array
    // Returns fixed-size array to avoid heap allocation
    std::array<uint8_t, 6> macStrToBytes(const std::string& mac) const {
        std::array<uint8_t, 6> bytes;
        
        // Expected format: "XX:XX:XX:XX:XX:XX" (17 chars)
        if (mac.length() != 17) {
            throw std::invalid_argument("Invalid MAC address format: expected XX:XX:XX:XX:XX:XX");
        }
        
        // Parse each byte pair
        for (size_t i = 0, byteIdx = 0; i < mac.length() && byteIdx < 6; i += 3, ++byteIdx) {
            // Check for colon separator (except after last byte)
            if (byteIdx > 0 && mac[i - 1] != ':') {
                throw std::invalid_argument("Invalid MAC address: missing colon separator");
            }
            
            // Parse two hex digits
            if (i + 1 >= mac.length()) {
                throw std::invalid_argument("Invalid MAC address: truncated byte");
            }
            
            try {
                bytes[byteIdx] = static_cast<uint8_t>(std::stoi(mac.substr(i, 2), nullptr, 16));
            } catch (const std::exception& e) {
                throw std::invalid_argument("Invalid MAC address: non-hex digit");
            }
        }
        
        return bytes;
    }

    std::vector<uint8_t> getEncoded() const {
        std::vector<uint8_t> encoded;
        encoded.reserve(12); // Pre-allocate for 2 MAC addresses

        auto dstBytes = macStrToBytes(macDst);
        auto srcBytes = macStrToBytes(macSrc);

        encoded.insert(encoded.end(), dstBytes.begin(), dstBytes.end());
        encoded.insert(encoded.end(), srcBytes.begin(), srcBytes.end());

        return encoded;
    }
};


#endif // ETHERNET_HPP
