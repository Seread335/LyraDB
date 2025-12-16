#include <gtest/gtest.h>
#include "lyradb/compression_selector.h"
#include "lyradb/delta_compressor.h"
#include "lyradb/bitpacking_compressor.h"
#include "lyradb/dict_compressor.h"

namespace lyradb {
namespace compression {
namespace test {

TEST(CompressionSelectorTest, SelectForIntegers_SortedData) {
    // Sorted data should select DELTA
    int64_t values[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    auto algo = CompressionSelector::select_for_integers(values, 10);
    EXPECT_EQ(algo, CompressionAlgorithm::DELTA);
}

TEST(CompressionSelectorTest, SelectForIntegers_SmallRange) {
    // Small range should select BITPACKING
    int64_t values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    
    auto algo = CompressionSelector::select_for_integers(values, 8);
    EXPECT_EQ(algo, CompressionAlgorithm::BITPACKING);
}

TEST(CompressionSelectorTest, SelectForIntegers_RandomData) {
    // Random large values should select ZSTD or UNCOMPRESSED
    int64_t values[] = {1000000, 2000000, 3000000, 4000000};
    
    auto algo = CompressionSelector::select_for_integers(values, 4, 0.99);
    // Should be UNCOMPRESSED or ZSTD for poor compression data
    EXPECT_TRUE(
        algo == CompressionAlgorithm::UNCOMPRESSED || 
        algo == CompressionAlgorithm::ZSTD
    );
}

TEST(CompressionSelectorTest, AlgorithmNameConversion) {
    EXPECT_STREQ(
        CompressionSelector::algorithm_name(CompressionAlgorithm::DELTA),
        "Delta Encoding");
    
    EXPECT_STREQ(
        CompressionSelector::algorithm_name(CompressionAlgorithm::BITPACKING),
        "Bitpacking");
    
    EXPECT_STREQ(
        CompressionSelector::algorithm_name(CompressionAlgorithm::RLE),
        "Run-Length Encoding");
    
    EXPECT_STREQ(
        CompressionSelector::algorithm_name(CompressionAlgorithm::DICTIONARY),
        "Dictionary Encoding");
}

TEST(CompressionSelectorTest, EstimateRatioForBitpacking) {
    int64_t values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    size_t byte_length = sizeof(values);
    
    double ratio = CompressionSelector::estimate_ratio(
        CompressionAlgorithm::BITPACKING,
        reinterpret_cast<const uint8_t*>(values),
        byte_length);
    
    // Small range should compress well
    EXPECT_LT(ratio, 1.0);
}

} // namespace test
} // namespace compression
} // namespace lyradb
