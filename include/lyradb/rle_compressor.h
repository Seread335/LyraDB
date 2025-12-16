#pragma once

#include <cstdint>
#include <vector>

namespace lyradb {
namespace compression {

/**
 * @brief Run-Length Encoding (RLE)
 * Optimal for repetitive data (many consecutive equal values)
 * 
 * Format: [value_type (1 byte)] [run_count (4 bytes)] [value (...)]
 * Example: 5 consecutive 42 -> [1] [5] [42]
 */
class RLECompressor {
public:
    /**
     * @brief Compress data using RLE
     * Best for integer arrays with repetition
     */
    static std::vector<uint8_t> compress(
        const uint8_t* data, 
        size_t length, 
        size_t value_size);
    
    /**
     * @brief Decompress RLE data
     */
    static std::vector<uint8_t> decompress(
        const uint8_t* data, 
        size_t length, 
        size_t value_size);
    
    /**
     * @brief Estimate if RLE is beneficial
     * Returns compression ratio or 1.0 if not beneficial
     */
    static double estimate_compression_ratio(
        const uint8_t* data, 
        size_t length, 
        size_t value_size);
    
private:
    struct RLESegment {
        uint32_t run_count;
        const uint8_t* value;
        
        size_t encoded_size(size_t value_size) const {
            return 4 + value_size;  // run_count (4 bytes) + value
        }
    };
    
    static std::vector<RLESegment> analyze_runs(
        const uint8_t* data, 
        size_t length, 
        size_t value_size);
};

} // namespace compression
} // namespace lyradb
