#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>

namespace lyradb {
namespace compression {

/**
 * @brief Compression algorithm selection strategy
 */
enum class CompressionAlgorithm {
    UNCOMPRESSED = 0,
    RLE = 1,
    DICTIONARY = 2,
    BITPACKING = 3,
    DELTA = 4,
    ZSTD = 5
};

/**
 * @brief Adaptive compression selector
 * Automatically chooses the best compression algorithm based on data characteristics
 */
class CompressionSelector {
public:
    /**
     * @brief Select best compression for integer array
     * @param values Pointer to integer values
     * @param count Number of values
     * @param min_compression_ratio Minimum compression benefit to apply compression (0.95 = 5% reduction required)
     * @return Selected algorithm
     */
    static CompressionAlgorithm select_for_integers(
        const int64_t* values,
        size_t count,
        double min_compression_ratio = 0.95);
    
    /**
     * @brief Select best compression for binary data with repetition
     * @param data Pointer to binary data
     * @param length Data length in bytes
     * @param value_size Size of each value in bytes
     * @param min_compression_ratio Minimum compression benefit to apply compression
     * @return Selected algorithm
     */
    static CompressionAlgorithm select_for_binary(
        const uint8_t* data,
        size_t length,
        size_t value_size,
        double min_compression_ratio = 0.95);
    
    /**
     * @brief Select best compression for string data
     * @param values Vector of string values
     * @param min_compression_ratio Minimum compression benefit to apply compression
     * @return Selected algorithm
     */
    static CompressionAlgorithm select_for_strings(
        const std::vector<std::string>& values,
        double min_compression_ratio = 0.95);
    
    /**
     * @brief Get human-readable name for algorithm
     */
    static const char* algorithm_name(CompressionAlgorithm algo);
    
    /**
     * @brief Get estimated compression ratio for algorithm
     * Helper method combining all estimators
     */
    static double estimate_ratio(
        CompressionAlgorithm algo,
        const uint8_t* data,
        size_t length,
        size_t value_size = 0);

private:
    // Threshold constants for algorithm suitability
    static constexpr double DELTA_SORTEDNESS_THRESHOLD = 0.80;  // 80% of values must be ascending
    static constexpr double DICT_CARDINALITY_THRESHOLD = 0.10;  // < 10% unique values
    static constexpr double RLE_RUN_EFFICIENCY_THRESHOLD = 0.70; // Average run length > 1.4
};

} // namespace compression
} // namespace lyradb
