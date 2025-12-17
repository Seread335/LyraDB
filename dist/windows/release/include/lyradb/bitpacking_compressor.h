#pragma once

#include <cstdint>
#include <vector>
#include <cmath>

namespace lyradb {
namespace compression {

/**
 * @brief Bitpacking Compression
 * Optimal for integer arrays with limited range
 * 
 * Stores values using only the number of bits needed for the max value
 * Example: values [0, 3, 5, 7] need 3 bits each (max 7 = 2^3 - 1)
 * 
 * Format:
 * - Header: [bit_width (1 byte)] [num_values (4 bytes)] [min_value (8 bytes)]
 * - Data: Packed bits containing (value - min_value)
 */
class BitpackingCompressor {
public:
    /**
     * @brief Compress integer array using bitpacking
     * @param values Array of int64 values
     */
    static std::vector<uint8_t> compress(
        const int64_t* values, 
        size_t count);
    
    /**
     * @brief Decompress bitpacked data
     */
    static std::vector<int64_t> decompress(
        const uint8_t* data, 
        size_t length);
    
    /**
     * @brief Estimate compression ratio
     * Returns compression ratio (< 1.0 means beneficial)
     */
    static double estimate_compression_ratio(
        const int64_t* values, 
        size_t count);
    
    /**
     * @brief Calculate bits needed to store a value
     */
    static uint8_t calculate_bit_width(int64_t max_value);

private:
    struct BitpackHeader {
        uint8_t bit_width;      // Bits needed per value
        uint32_t num_values;    // Number of values
        int64_t min_value;      // Min value (for delta encoding)
        
        static constexpr size_t SIZE = 1 + 4 + 8;  // 13 bytes
    };
    
    /**
     * @brief Write value to bit buffer at specific bit position
     */
    static void write_bits(
        uint8_t* buffer, 
        size_t bit_offset, 
        uint64_t value, 
        uint8_t bit_width);
    
    /**
     * @brief Read value from bit buffer at specific bit position
     */
    static uint64_t read_bits(
        const uint8_t* buffer, 
        size_t bit_offset, 
        uint8_t bit_width);
};

} // namespace compression
} // namespace lyradb
