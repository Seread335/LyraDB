#include <gtest/gtest.h>
#include "lyradb/delta_compressor.h"

namespace lyradb {
namespace compression {
namespace test {

TEST(DeltaCompressorTest, ZigzagEncode) {
    // Zigzag: 0->0, -1->1, 1->2, -2->3, 2->4, etc.
    EXPECT_EQ(DeltaCompressor::zigzag_encode(0), 0ULL);
    EXPECT_EQ(DeltaCompressor::zigzag_encode(-1), 1ULL);
    EXPECT_EQ(DeltaCompressor::zigzag_encode(1), 2ULL);
    EXPECT_EQ(DeltaCompressor::zigzag_encode(-2), 3ULL);
}

TEST(DeltaCompressorTest, ZigzagDecode) {
    EXPECT_EQ(DeltaCompressor::zigzag_decode(0), 0LL);
    EXPECT_EQ(DeltaCompressor::zigzag_decode(1), -1LL);
    EXPECT_EQ(DeltaCompressor::zigzag_decode(2), 1LL);
    EXPECT_EQ(DeltaCompressor::zigzag_decode(3), -2LL);
}

TEST(DeltaCompressorTest, CompressSorted) {
    int64_t values[] = {100, 102, 104, 101, 105, 103};
    size_t count = sizeof(values) / sizeof(values[0]);
    
    auto compressed = DeltaCompressor::compress(values, count);
    EXPECT_GT(compressed.size(), 0);
}

TEST(DeltaCompressorTest, DecompressSorted) {
    int64_t values[] = {100, 102, 104, 106, 108, 110};
    size_t count = sizeof(values) / sizeof(values[0]);
    
    auto compressed = DeltaCompressor::compress(values, count);
    auto decompressed = DeltaCompressor::decompress(
        compressed.data(),
        compressed.size());
    
    EXPECT_EQ(decompressed.size(), count);
    for (size_t i = 0; i < count; ++i) {
        EXPECT_EQ(decompressed[i], values[i]);
    }
}

TEST(DeltaCompressorTest, IsSuitable) {
    // Sorted data - suitable
    int64_t sorted[] = {1, 2, 3, 4, 5, 6};
    EXPECT_TRUE(DeltaCompressor::is_suitable(sorted, 6));
    
    // Random data - not suitable
    int64_t random[] = {100, 5, 50, 1, 80, 20};
    EXPECT_FALSE(DeltaCompressor::is_suitable(random, 6));
}

} // namespace test
} // namespace compression
} // namespace lyradb
