#include <gtest/gtest.h>
#include "lyradb/rle_compressor.h"
#include <cstring>
#include <vector>

using namespace lyradb::compression;

class RLECompressionTest : public ::testing::Test {
protected:
    // Helper to create test data
    static std::vector<uint8_t> create_int64_data(const std::vector<int64_t>& values) {
        std::vector<uint8_t> data;
        for (int64_t v : values) {
            uint8_t bytes[8];
            std::memcpy(bytes, &v, sizeof(int64_t));
            data.insert(data.end(), bytes, bytes + 8);
        }
        return data;
    }
    
    static std::vector<int64_t> extract_int64_data(const std::vector<uint8_t>& data) {
        std::vector<int64_t> values;
        for (size_t i = 0; i + 7 < data.size(); i += 8) {
            int64_t v;
            std::memcpy(&v, &data[i], sizeof(int64_t));
            values.push_back(v);
        }
        return values;
    }
};

// ============================================================================
// Basic Compression/Decompression Tests
// ============================================================================

TEST_F(RLECompressionTest, CompressEmptyData) {
    auto compressed = RLECompressor::compress(nullptr, 0, 8);
    EXPECT_TRUE(compressed.empty());
}

TEST_F(RLECompressionTest, CompressDecompressSimpleRuns) {
    // Data: 5 copies of 42, 3 copies of 99, 2 copies of 1
    auto data = create_int64_data({42, 42, 42, 42, 42, 99, 99, 99, 1, 1});
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    EXPECT_LT(compressed.size(), data.size());  // Should compress well
    
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    EXPECT_EQ(data, decompressed);
    
    auto original = extract_int64_data(data);
    auto recovered = extract_int64_data(decompressed);
    EXPECT_EQ(original, recovered);
}

TEST_F(RLECompressionTest, CompressDecompressHighlyRepetitive) {
    // 100 copies of same value
    std::vector<int64_t> original(100, 42);
    auto data = create_int64_data(original);
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    EXPECT_LT(compressed.size(), data.size());  // Significant compression
    
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    EXPECT_EQ(data, decompressed);
}

TEST_F(RLECompressionTest, CompressDecompressNoRepetition) {
    // All different values - worst case for RLE
    std::vector<int64_t> original = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    auto data = create_int64_data(original);
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    // May be larger or equal due to overhead
    
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    EXPECT_EQ(data, decompressed);
}

TEST_F(RLECompressionTest, CompressDecompressInterspersedRuns) {
    // Pattern: 5x10, 3x20, 2x30, 7x40
    std::vector<int64_t> original;
    for (int i = 0; i < 5; ++i) original.push_back(10);
    for (int i = 0; i < 3; ++i) original.push_back(20);
    for (int i = 0; i < 2; ++i) original.push_back(30);
    for (int i = 0; i < 7; ++i) original.push_back(40);
    
    auto data = create_int64_data(original);
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
    EXPECT_LT(compressed.size(), data.size());
}

TEST_F(RLECompressionTest, CompressDecompressLargeValues) {
    // Large values (shouldn't affect compression)
    std::vector<int64_t> original = {
        0x0102030405060708LL, 0x0102030405060708LL, 0x0102030405060708LL,
        0x0A0B0C0D0E0F0001LL, 0x0A0B0C0D0E0F0001LL
    };
    
    auto data = create_int64_data(original);
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
}

// ============================================================================
// Compression Ratio Tests
// ============================================================================

TEST_F(RLECompressionTest, CompressionRatioHighlyRepetitive) {
    std::vector<int64_t> original(1000, 42);  // 1000 identical values
    auto data = create_int64_data(original);
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    double ratio = static_cast<double>(compressed.size()) / data.size();
    
    EXPECT_LT(ratio, 0.1);  // Should compress to <10% of original
}

TEST_F(RLECompressionTest, CompressionRatioModerateRepetition) {
    std::vector<int64_t> original;
    for (int i = 0; i < 10; ++i) {
        for (int j = 0; j < 50; ++j) {  // 50 copies of each value
            original.push_back(i);
        }
    }
    
    auto data = create_int64_data(original);
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    double ratio = static_cast<double>(compressed.size()) / data.size();
    
    EXPECT_LT(ratio, 0.3);  // Should compress to <30% of original
}

TEST_F(RLECompressionTest, CompressionRatioNoRepetition) {
    std::vector<int64_t> original;
    for (int i = 0; i < 1000; ++i) {
        original.push_back(i);  // All different
    }
    
    auto data = create_int64_data(original);
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    double ratio = static_cast<double>(compressed.size()) / data.size();
    
    EXPECT_GE(ratio, 1.0);  // May expand due to overhead
}

// ============================================================================
// Value Size Tests
// ============================================================================

TEST_F(RLECompressionTest, CompressDecompressSmallValueSize) {
    // Single byte values: 4x'A', 3x'B', 2x'C'
    std::vector<uint8_t> data = {'A', 'A', 'A', 'A', 'B', 'B', 'B', 'C', 'C'};
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 1);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 1);
    
    EXPECT_EQ(data, decompressed);
}

TEST_F(RLECompressionTest, CompressDecompressLargeValueSize) {
    // 4-byte values
    std::vector<uint32_t> original = {100, 100, 100, 200, 200, 300};
    std::vector<uint8_t> data;
    for (uint32_t v : original) {
        uint8_t bytes[4];
        std::memcpy(bytes, &v, sizeof(uint32_t));
        data.insert(data.end(), bytes, bytes + 4);
    }
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 4);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 4);
    
    EXPECT_EQ(data, decompressed);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(RLECompressionTest, DecompressInvalidData) {
    std::vector<uint8_t> invalid_data = {0x01, 0x02};  // Too short
    
    EXPECT_THROW(
        RLECompressor::decompress(invalid_data.data(), invalid_data.size(), 8),
        std::runtime_error
    );
}

TEST_F(RLECompressionTest, CompressInvalidValueSize) {
    auto data = create_int64_data({1, 2, 3});
    
    EXPECT_THROW(
        RLECompressor::compress(data.data(), data.size(), 0),
        std::runtime_error
    );
}

TEST_F(RLECompressionTest, CompressLengthNotMultipleOfValueSize) {
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};  // 5 bytes, not divisible by 8
    
    EXPECT_THROW(
        RLECompressor::compress(data.data(), data.size(), 8),
        std::runtime_error
    );
}

// ============================================================================
// Ratio Estimation Tests
// ============================================================================

TEST_F(RLECompressionTest, EstimateRatioHighlyRepetitive) {
    std::vector<int64_t> original(100, 42);
    auto data = create_int64_data(original);
    
    double ratio = RLECompressor::estimate_compression_ratio(data.data(), data.size(), 8);
    EXPECT_LT(ratio, 0.2);  // Should be good for compression
}

TEST_F(RLECompressionTest, EstimateRatioNoRepetition) {
    std::vector<int64_t> original;
    for (int i = 0; i < 100; ++i) {
        original.push_back(i);
    }
    auto data = create_int64_data(original);
    
    double ratio = RLECompressor::estimate_compression_ratio(data.data(), data.size(), 8);
    EXPECT_GT(ratio, 0.8);  // Should be poor for compression
}

TEST_F(RLECompressionTest, EstimateRatioMixedPattern) {
    std::vector<int64_t> original;
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (int i = 0; i < 5; ++i) {
            original.push_back(cycle);
        }
    }
    auto data = create_int64_data(original);
    
    double ratio = RLECompressor::estimate_compression_ratio(data.data(), data.size(), 8);
    EXPECT_GT(ratio, 0.1);
    EXPECT_LT(ratio, 0.5);
}

TEST_F(RLECompressionTest, EstimateRatioEmptyData) {
    double ratio = RLECompressor::estimate_compression_ratio(nullptr, 0, 8);
    EXPECT_EQ(ratio, 1.0);
}

// ============================================================================
// Large Data Tests
// ============================================================================

TEST_F(RLECompressionTest, CompressDecompress10MB) {
    // 1 million int64_t values (8 MB), with repetitive pattern
    std::vector<int64_t> original;
    for (int cycle = 0; cycle < 1000; ++cycle) {
        for (int i = 0; i < 1000; ++i) {
            original.push_back(i % 10);  // 10 unique values, repeated
        }
    }
    
    auto data = create_int64_data(original);
    EXPECT_EQ(data.size(), 8000000);  // 1M values * 8 bytes
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    EXPECT_LT(compressed.size(), data.size());
    
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    EXPECT_EQ(data, decompressed);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(RLECompressionTest, SingleValue) {
    auto data = create_int64_data({42});
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
}

TEST_F(RLECompressionTest, TwoIdenticalValues) {
    auto data = create_int64_data({42, 42});
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
}

TEST_F(RLECompressionTest, TwoDifferentValues) {
    auto data = create_int64_data({42, 99});
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
}

TEST_F(RLECompressionTest, AlternatingValues) {
    std::vector<int64_t> original;
    for (int i = 0; i < 100; ++i) {
        original.push_back(i % 2);  // Alternates between 0 and 1
    }
    
    auto data = create_int64_data(original);
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
    // Alternating should NOT compress well
    EXPECT_GE(compressed.size(), data.size() * 0.8);
}

TEST_F(RLECompressionTest, ZeroValues) {
    std::vector<int64_t> original(50, 0);
    auto data = create_int64_data(original);
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    EXPECT_LT(compressed.size(), data.size());
    
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    EXPECT_EQ(data, decompressed);
}

TEST_F(RLECompressionTest, NegativeValues) {
    std::vector<int64_t> original = {-1, -1, -1, 0, 0, 1, 1};
    auto data = create_int64_data(original);
    
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
    EXPECT_EQ(extract_int64_data(decompressed), original);
}

// ============================================================================
// High Value Range Tests
// ============================================================================

TEST_F(RLECompressionTest, MaxInt64Values) {
    std::vector<int64_t> original = {
        9223372036854775807LL, 9223372036854775807LL,  // max int64_t
        -9223372036854775807LL, -9223372036854775807LL  // min int64_t
    };
    
    auto data = create_int64_data(original);
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
    EXPECT_EQ(extract_int64_data(decompressed), original);
}

TEST_F(RLECompressionTest, BinaryPatterns) {
    std::vector<int64_t> original;
    for (int i = 0; i < 32; ++i) {
        int64_t v = 1LL << i;
        for (int j = 0; j < 3; ++j) {
            original.push_back(v);
        }
    }
    
    auto data = create_int64_data(original);
    auto compressed = RLECompressor::compress(data.data(), data.size(), 8);
    auto decompressed = RLECompressor::decompress(compressed.data(), compressed.size(), 8);
    
    EXPECT_EQ(data, decompressed);
}
