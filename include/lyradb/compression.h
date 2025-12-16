#pragma once

#include <vector>
#include <cstdint>

namespace lyradb {

/**
 * @brief Compression engine with multiple algorithms
 * ZSTD, RLE, Bitpacking, Delta, Dictionary encoding
 */
enum class CompressionType : uint8_t {
    NONE = 0,
    ZSTD = 1,
    RLE = 2,
    DICTIONARY = 3,
    BITPACKING = 4,
    DELTA = 5,
};

class Compression {
public:
    static std::vector<uint8_t> compress(
        const std::vector<uint8_t>& data,
        CompressionType type);
    
    static std::vector<uint8_t> decompress(
        const std::vector<uint8_t>& data,
        CompressionType type);
    
    // Individual compression methods
    static std::vector<uint8_t> compress_zstd(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompress_zstd(const std::vector<uint8_t>& data);
    
    static std::vector<uint8_t> compress_rle(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompress_rle(const std::vector<uint8_t>& data);
    
    static std::vector<uint8_t> compress_dictionary(const std::vector<uint8_t>& data);
    static std::vector<uint8_t> decompress_dictionary(const std::vector<uint8_t>& data);
};

} // namespace lyradb
