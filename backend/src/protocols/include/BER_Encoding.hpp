#ifndef BER_ENCODING_HPP
#define BER_ENCODING_HPP

#include <vector>
#include <cstdint>
#include <stdexcept>

// Helper function to encode BER length according to ITU-T X.690
// Short form (length ≤ 127): single byte
// Long form (length > 127): 0x81 + 1 byte or 0x82 + 2 bytes
inline std::vector<uint8_t> encodeBERLength(size_t length) {
    std::vector<uint8_t> encoded;
    if (length <= 127) {
        // Short form: single byte
        encoded.push_back(static_cast<uint8_t>(length));
    } else if (length <= 255) {
        // Long form with 1 byte: 0x81 + length byte
        encoded.push_back(0x81);
        encoded.push_back(static_cast<uint8_t>(length));
    } else if (length <= 65535) {
        // Long form with 2 bytes: 0x82 + 2 length bytes (big-endian)
        encoded.push_back(0x82);
        encoded.push_back(static_cast<uint8_t>((length >> 8) & 0xFF));
        encoded.push_back(static_cast<uint8_t>(length & 0xFF));
    } else {
        throw std::runtime_error("BER length > 65535 not supported");
    }
    return encoded;
}

#endif // BER_ENCODING_HPP
