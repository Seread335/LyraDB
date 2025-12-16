#include <gtest/gtest.h>
#include "lyradb/bitpacking_compressor.h"

namespace lyradb {
namespace compression {
namespace test {

TEST(BitpackingCompressorTest, CalculateBitWidth) {
    // 0-1 needs 1 bit
    EXPECT_EQ(BitpackingCompressor::calculate_bit_width(1), 1);
    
    // 0-3 needs 2 bits
    EXPECT_EQ(BitpackingCompressor::calculate_bit_width(3), 2);
    
    // 0-7 needs 3 bits
    EXPECT_EQ(BitpackingCompressor::calculate_bit_width(7), 3);
    
    // 0-255 needs 8 bits
    EXPECT_EQ(BitpackingCompressor::calculate_bit_width(255), 8);
}

TEST(BitpackingCompressorTest, CompressSmallRange) {
    // Values with small range: all fit in 4 bits
    int64_t values[] = {0, 5, 10, 15, 8, 3};
    size_t count = sizeof(values) / sizeof(values[0]);
    
    auto compressed = BitpackingCompressor::compress(values, count);
    
    // Should be much smaller than original (6 * 8 = 48 bytes)
    EXPECT_LT(compressed.size(), 48);
    
    // Decompress and verify
    auto decompressed = BitpackingCompressor::decompress(
        compressed.data(),
        compressed.size());
    
    EXPECT_EQ(decompressed.size(), count);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(decompressed[i], values[i]);
    }
}

TEST(BitpackingCompressorTest, EstimateRatio) {
    int64_t values[] = {0, 1, 2, 3, 4, 5, 6, 7};
    size_t count = sizeof(values) / sizeof(values[0]);
    
    double ratio = BitpackingCompressor::estimate_compression_ratio(
        values, count);
    
    // Small range should compress well
    EXPECT_LT(ratio, 0.5);
}

} // namespace test
} // namespace compression
} // namespace lyradb
