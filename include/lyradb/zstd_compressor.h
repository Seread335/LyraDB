#pragma once

#include <cstdint>
#include <vector>
#include <cstring>

namespace lyradb {
namespace compression {

/**
 * @brief ZSTD Compression (External library wrapper)
 * General-purpose compression for mixed data types
 * 
 * ZSTD (Zstandard) is a modern real-time compression algorithm.
 * It provides high compression ratios at high speeds.
 * 
 * Compression Levels: 1-22
 * - Levels 1-3: Fast, lower ratio (default: 3)
 * - Levels 10-15: Balanced
 * - Levels 18-22: Slow, higher ratio
 */
class ZstdCompressor {
public:
    /**
     * Compression level: 1-22 (default 3)
     * Higher = better compression but slower
     * 
     * @param level Compression level (1-22)
     * @throws std::runtime_error if level is out of range
     */
    explicit ZstdCompressor(int level = 3);
    
    /**
     * @brief Compress data using ZSTD
     * 
     * @param data Input data pointer
     * @param length Input data length
     * @return Compressed data vector (or original if not beneficial)
     * 
     * @note Will return original data if:
     *   - Input is too small (<100 bytes)
     *   - Compression would increase size
     *   - Compression fails
     */
    std::vector<uint8_t> compress(const uint8_t* data, size_t length) const;
    
    /**
     * @brief Decompress ZSTD-compressed data
     * 
     * @param data Compressed data pointer
     * @param length Compressed data length
     * @return Original decompressed data
     * 
     * @throws std::runtime_error if decompression fails
     */
    static std::vector<uint8_t> decompress(const uint8_t* data, size_t length);
    
    /**
     * @brief Estimate compression ratio for decision-making
     * 
     * Samples up to 64KB of data to estimate compression effectiveness.
     * 
     * @param data Input data pointer
     * @param length Input data length
     * @return Estimated compression ratio (0.01 - 1.5)
     *   - 0.5 means 50% compression (half the size)
     *   - 1.0 means no compression
     *   - 1.5 means 50% expansion
     */
    static double estimate_ratio(const uint8_t* data, size_t length);
    
private:
    int level_;
    static constexpr size_t ZSTD_WINDOW_SIZE = 128 * 1024;  // 128 KB
};

} // namespace compression
} // namespace lyradb
