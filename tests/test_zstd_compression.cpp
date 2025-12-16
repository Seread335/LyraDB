/**
 * ZSTD Compression Tests
 * 
 * Tests for ZSTD compression/decompression functionality
 */

#include <gtest/gtest.h>
#include "lyradb/zstd_compressor.h"
#include <random>
#include <cstring>

using namespace lyradb::compression;

class ZstdCompressionTest : public ::testing::Test {
protected:
    ZstdCompressor default_compressor{3};  // Default level
    ZstdCompressor fast_compressor{1};     // Fast compression
    ZstdCompressor strong_compressor{22};  // Strong compression
    
    // Generate test data
    std::vector<uint8_t> generate_random_data(size_t size) {
        std::vector<uint8_t> data(size);
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }
    
    // Generate repetitive data (compresses well)
    std::vector<uint8_t> generate_repetitive_data(size_t size) {
        std::vector<uint8_t> data(size);
        for (size_t i = 0; i < size; i++) {
            data[i] = static_cast<uint8_t>(i % 256);
        }
        return data;
    }
    
    // Generate text-like data (should compress well)
    std::vector<uint8_t> generate_text_data(size_t size) {
        std::string text = "The quick brown fox jumps over the lazy dog. ";
        std::vector<uint8_t> data;
        
        while (data.size() < size) {
            data.insert(data.end(), text.begin(), text.end());
        }
        data.resize(size);
        return data;
    }
};

// ============================================================================
// Basic Compression/Decompression Tests
// ============================================================================

TEST_F(ZstdCompressionTest, CompressEmptyData) {
    std::vector<uint8_t> data;
    auto compressed = default_compressor.compress(data.data(), data.size());
    EXPECT_EQ(compressed.size(), 0);
}

TEST_F(ZstdCompressionTest, CompressSmallData) {
    // Very small data shouldn't be worth compressing
    std::vector<uint8_t> data = {1, 2, 3, 4, 5};
    auto compressed = default_compressor.compress(data.data(), data.size());
    // Should return original or something small
    EXPECT_LE(compressed.size(), data.size() + 20);  // Allow for minimal overhead
}

TEST_F(ZstdCompressionTest, CompressDecompressRoundtrip) {
    std::vector<uint8_t> original = generate_repetitive_data(10000);
    
    auto compressed = default_compressor.compress(original.data(), original.size());
    auto decompressed = ZstdCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(original, decompressed);
}

TEST_F(ZstdCompressionTest, CompressDecompressRandomData) {
    std::vector<uint8_t> original = generate_random_data(50000);
    
    auto compressed = default_compressor.compress(original.data(), original.size());
    auto decompressed = ZstdCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(original, decompressed);
}

TEST_F(ZstdCompressionTest, CompressDecompressTextData) {
    std::vector<uint8_t> original = generate_text_data(100000);
    
    auto compressed = default_compressor.compress(original.data(), original.size());
    auto decompressed = ZstdCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(original, decompressed);
}

// ============================================================================
// Compression Ratio Tests
// ============================================================================

TEST_F(ZstdCompressionTest, HighCompressionRatioForRepetitive) {
    std::vector<uint8_t> data = generate_repetitive_data(100000);
    
    auto compressed = default_compressor.compress(data.data(), data.size());
    
    double ratio = static_cast<double>(compressed.size()) / data.size();
    
    // Repetitive data should compress to < 30% of original
    EXPECT_LT(ratio, 0.3) << "Ratio: " << ratio;
}

TEST_F(ZstdCompressionTest, ModerateCompressionRatioForText) {
    std::vector<uint8_t> data = generate_text_data(100000);
    
    auto compressed = default_compressor.compress(data.data(), data.size());
    
    double ratio = static_cast<double>(compressed.size()) / data.size();
    
    // Text should compress to 30-70% of original
    EXPECT_GT(ratio, 0.2) << "Ratio: " << ratio;
    EXPECT_LT(ratio, 0.8) << "Ratio: " << ratio;
}

TEST_F(ZstdCompressionTest, PoorCompressionRatioForRandom) {
    std::vector<uint8_t> data = generate_random_data(100000);
    
    auto compressed = default_compressor.compress(data.data(), data.size());
    
    double ratio = static_cast<double>(compressed.size()) / data.size();
    
    // Random data should not compress well (>0.99 ratio)
    EXPECT_GT(ratio, 0.95) << "Ratio: " << ratio;
}

// ============================================================================
// Compression Level Tests
// ============================================================================

TEST_F(ZstdCompressionTest, FasterCompressionVsStronger) {
    std::vector<uint8_t> data = generate_text_data(1000000);
    
    auto compressed_fast = fast_compressor.compress(data.data(), data.size());
    auto compressed_strong = strong_compressor.compress(data.data(), data.size());
    
    // Strong compression should be smaller than fast compression
    EXPECT_LE(compressed_strong.size(), compressed_fast.size())
        << "Strong: " << compressed_strong.size() 
        << " Fast: " << compressed_fast.size();
}

TEST_F(ZstdCompressionTest, AllLevelsProduceValidOutput) {
    std::vector<uint8_t> data = generate_text_data(10000);
    
    for (int level = 1; level <= 22; level++) {
        ZstdCompressor compressor(level);
        auto compressed = compressor.compress(data.data(), data.size());
        auto decompressed = ZstdCompressor::decompress(compressed.data(), compressed.size());
        
        EXPECT_EQ(data, decompressed) << "Level " << level << " failed";
    }
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(ZstdCompressionTest, InvalidCompressionLevel) {
    EXPECT_THROW({
        ZstdCompressor compressor(0);  // Invalid level
    }, std::runtime_error);
    
    EXPECT_THROW({
        ZstdCompressor compressor(23);  // Invalid level
    }, std::runtime_error);
}

TEST_F(ZstdCompressionTest, DecompressInvalidData) {
    std::vector<uint8_t> garbage = {0xFF, 0xFE, 0xFD, 0xFC};
    
    EXPECT_THROW({
        ZstdCompressor::decompress(garbage.data(), garbage.size());
    }, std::runtime_error);
}

TEST_F(ZstdCompressionTest, DecompressTruncatedData) {
    std::vector<uint8_t> original = generate_text_data(10000);
    auto compressed = default_compressor.compress(original.data(), original.size());
    
    // Truncate the compressed data
    if (compressed.size() > 10) {
        compressed.resize(compressed.size() - 10);
        
        EXPECT_THROW({
            ZstdCompressor::decompress(compressed.data(), compressed.size());
        }, std::runtime_error);
    }
}

// ============================================================================
// Ratio Estimation Tests
// ============================================================================

TEST_F(ZstdCompressionTest, EstimateRatioRepetitive) {
    std::vector<uint8_t> data = generate_repetitive_data(100000);
    
    double estimated = ZstdCompressor::estimate_ratio(data.data(), data.size());
    
    // Estimate should be reasonable (between 0.01 and 1.0)
    EXPECT_GT(estimated, 0.01);
    EXPECT_LT(estimated, 0.5);
}

TEST_F(ZstdCompressionTest, EstimateRatioRandom) {
    std::vector<uint8_t> data = generate_random_data(100000);
    
    double estimated = ZstdCompressor::estimate_ratio(data.data(), data.size());
    
    // Random data should have estimate close to 1.0
    EXPECT_GT(estimated, 0.95);
    EXPECT_LE(estimated, 1.5);
}

TEST_F(ZstdCompressionTest, EstimateRatioSmallData) {
    std::vector<uint8_t> data = generate_random_data(50);
    
    double estimated = ZstdCompressor::estimate_ratio(data.data(), data.size());
    
    // Should be reasonable even for small data
    EXPECT_GT(estimated, 0.01);
    EXPECT_LE(estimated, 1.5);
}

TEST_F(ZstdCompressionTest, EstimateRatioEmptyData) {
    double estimated = ZstdCompressor::estimate_ratio(nullptr, 0);
    
    EXPECT_EQ(estimated, 1.0);
}

// ============================================================================
// Large Data Tests
// ============================================================================

TEST_F(ZstdCompressionTest, CompressLargeData) {
    // Test with 10MB of data
    std::vector<uint8_t> data = generate_text_data(10 * 1024 * 1024);
    
    auto compressed = default_compressor.compress(data.data(), data.size());
    auto decompressed = ZstdCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(data, decompressed);
    
    double ratio = static_cast<double>(compressed.size()) / data.size();
    EXPECT_LT(ratio, 0.8) << "Large data should compress reasonably";
}

// ============================================================================
// Null Pointer Tests
// ============================================================================

TEST_F(ZstdCompressionTest, CompressNullPointer) {
    auto compressed = default_compressor.compress(nullptr, 100);
    EXPECT_EQ(compressed.size(), 0);
}

TEST_F(ZstdCompressionTest, DecompressNullPointer) {
    std::vector<uint8_t> result = ZstdCompressor::decompress(nullptr, 100);
    EXPECT_EQ(result.size(), 0);
}

// ============================================================================
// Binary Data Tests
// ============================================================================

TEST_F(ZstdCompressionTest, CompressBinaryData) {
    // Create binary data with various byte patterns
    std::vector<uint8_t> data;
    for (int i = 0; i < 100000; i++) {
        data.push_back(static_cast<uint8_t>(i & 0xFF));
    }
    
    auto compressed = default_compressor.compress(data.data(), data.size());
    auto decompressed = ZstdCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(data, decompressed);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
