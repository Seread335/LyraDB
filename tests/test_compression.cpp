#include <gtest/gtest.h>
#include "lyradb/rle_compressor.h"
#include <cstring>

namespace lyradb {
namespace compression {
namespace test {

TEST(RLECompressorTest, CompressIdenticalValues) {
    // Test data: 5 consecutive values of 42
    int32_t values[] = {42, 42, 42, 42, 42};
    size_t data_size = sizeof(values);
    
    auto compressed = RLECompressor::compress(
        reinterpret_cast<uint8_t*>(values),
        data_size,
        sizeof(int32_t));
    
    // Should be much smaller than original
    EXPECT_LT(compressed.size(), data_size);
}

TEST(RLECompressorTest, CompressMixedValues) {
    // Test data with varying values
    int32_t values[] = {1, 1, 1, 2, 2, 3, 3, 3, 3};
    size_t data_size = sizeof(values);
    
    auto compressed = RLECompressor::compress(
        reinterpret_cast<uint8_t*>(values),
        data_size,
        sizeof(int32_t));
    
    // Should compress reasonably well
    EXPECT_LT(compressed.size(), data_size);
    
    // Decompress and verify
    auto decompressed = RLECompressor::decompress(
        compressed.data(),
        compressed.size(),
        sizeof(int32_t));
    
    EXPECT_EQ(decompressed.size(), data_size);
    EXPECT_EQ(std::memcmp(
        reinterpret_cast<uint8_t*>(values),
        decompressed.data(),
        data_size), 0);
}

TEST(RLECompressorTest, EstimateRatio) {
    int32_t values[] = {42, 42, 42, 42, 42};
    size_t data_size = sizeof(values);
    
    double ratio = RLECompressor::estimate_compression_ratio(
        reinterpret_cast<uint8_t*>(values),
        data_size,
        sizeof(int32_t));
    
    // Should estimate good compression for repetitive data
    EXPECT_LT(ratio, 0.5);
}

} // namespace test
} // namespace compression
} // namespace lyradb
