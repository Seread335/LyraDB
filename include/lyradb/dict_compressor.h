#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

namespace lyradb {
namespace compression {

/**
 * @brief Dictionary Compression
 * Optimal for string data or categorical data with limited unique values
 * 
 * Format: 
 * - Dictionary header: [num_entries (4 bytes)]
 * - Dictionary entries: [key_len (2)] [key_bytes] [value_id (4)]
 * - Compressed data: [value_id (4)] for each value
 */
class DictionaryCompressor {
public:
    /**
     * @brief Compress data using dictionary encoding
     * Builds dictionary of unique values and replaces with IDs
     */
    static std::vector<uint8_t> compress(
        const std::vector<std::string>& values);
    
    /**
     * @brief Decompress dictionary-encoded data
     */
    static std::vector<std::string> decompress(
        const uint8_t* data, 
        size_t length);
    
    /**
     * @brief Estimate compression benefit
     * Returns compression ratio (< 1.0 means beneficial)
     */
    static double estimate_compression_ratio(
        const std::vector<std::string>& values);
    
    /**
     * @brief Check if dictionary compression is suitable
     * Returns true if unique value count / total < threshold
     */
    static bool is_suitable(
        const std::vector<std::string>& values,
        double cardinality_threshold = 0.1);

private:
    /**
     * @brief Dictionary entry in memory
     */
    struct DictEntry {
        std::string key;
        uint32_t id;
        uint32_t frequency;  // For optimization
    };
    
    /**
     * @brief Build optimal dictionary
     * Sorts entries by frequency for better compression
     */
    static std::vector<DictEntry> build_dictionary(
        const std::vector<std::string>& values);
};

} // namespace compression
} // namespace lyradb
