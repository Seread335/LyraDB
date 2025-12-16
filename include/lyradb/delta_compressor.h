#pragma once

#include <cstdint>
#include <vector>

namespace lyradb {
namespace compression {

/**
 * @brief Delta Encoding for Time-series Data
 * Stores differences between consecutive values instead of absolute values
 * 
 * Example: [100, 102, 104, 101] -> [100, 2, 2, -3]
 * 
 * Format:
 * - Header: [first_value (8 bytes)] [num_values (4 bytes)]
 * - Data: Delta-encoded values
 */
class DeltaCompressor {
public:
    /**
     * @brief Compress using delta encoding
     * Optimal for sorted or nearly-sorted integer sequences
     */
    static std::vector<uint8_t> compress(
        const int64_t* values, 
        size_t count);
    
    /**
     * @brief Decompress delta-encoded data
     */
    static std::vector<int64_t> decompress(
        const uint8_t* data, 
        size_t length);
    
    /**
     * @brief Estimate compression benefit
     */
    static double estimate_compression_ratio(
        const int64_t* values, 
        size_t count);
    
    /**
     * @brief Check if data is suitable for delta encoding
     * Returns true if average delta is much smaller than value range
     */
    static bool is_suitable(
        const int64_t* values, 
        size_t count);

private:
    /**
     * @brief Zigzag encode a signed integer to unsigned
     * Maps: -1->1, -2->3, 0->0, 1->2, 2->4, etc.
     * Smaller absolute values = smaller unsigned values
     */
    static uint64_t zigzag_encode(int64_t value);
    static int64_t zigzag_decode(uint64_t value);
};

} // namespace compression
} // namespace lyradb
