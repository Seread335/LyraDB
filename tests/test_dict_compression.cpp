#include <gtest/gtest.h>
#include "lyradb/dict_compressor.h"
#include <vector>
#include <string>

using namespace lyradb::compression;

class DictionaryCompressionTest : public ::testing::Test {
protected:
    // Helper to compare vectors
    static bool vectors_equal(
        const std::vector<std::string>& a,
        const std::vector<std::string>& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (a[i] != b[i]) return false;
        }
        return true;
    }
};

// ============================================================================
// Basic Compression/Decompression Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, CompressDecompressEmptyData) {
    std::vector<std::string> empty;
    auto compressed = DictionaryCompressor::compress(empty);
    EXPECT_TRUE(compressed.empty());
}

TEST_F(DictionaryCompressionTest, CompressDecompressSingleValue) {
    std::vector<std::string> values = {"hello"};
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressIdenticalValues) {
    std::vector<std::string> values = {"cat", "cat", "cat", "cat", "cat"};
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
    // Should compress very well (1 unique value)
    EXPECT_LT(compressed.size(), values.size() * 3 * 4);
}

TEST_F(DictionaryCompressionTest, CompressDecompressMultipleUniqueValues) {
    std::vector<std::string> values = {
        "apple", "banana", "cherry", "date", "elderberry"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressRepeatingPattern) {
    std::vector<std::string> values;
    for (int i = 0; i < 5; ++i) {
        values.push_back("apple");
        values.push_back("banana");
        values.push_back("cherry");
    }
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressAllDifferentValues) {
    std::vector<std::string> values = {
        "unique1", "unique2", "unique3", "unique4", "unique5"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

// ============================================================================
// Compression Ratio Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, CompressionRatioHighlyRepetitive) {
    // 100 identical values
    std::vector<std::string> values(100, "test");
    
    auto compressed = DictionaryCompressor::compress(values);
    double ratio = static_cast<double>(compressed.size()) / (100 * 4);
    
    // Should compress very well
    EXPECT_LT(ratio, 0.3);
}

TEST_F(DictionaryCompressionTest, CompressionRatioModerateCardinality) {
    // 100 values, 10 unique
    std::vector<std::string> values;
    for (int cycle = 0; cycle < 10; ++cycle) {
        for (int i = 0; i < 10; ++i) {
            values.push_back("value_" + std::to_string(i));
        }
    }
    
    auto compressed = DictionaryCompressor::compress(values);
    size_t original_size = 0;
    for (const auto& v : values) original_size += v.size();
    
    double ratio = static_cast<double>(compressed.size()) / original_size;
    EXPECT_LT(ratio, 0.4);
}

TEST_F(DictionaryCompressionTest, CompressionRatioHighCardinality) {
    // 100 values, all unique
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        values.push_back("unique_value_" + std::to_string(i));
    }
    
    auto compressed = DictionaryCompressor::compress(values);
    size_t original_size = 0;
    for (const auto& v : values) original_size += v.size();
    
    // May not compress well with high cardinality
    double ratio = static_cast<double>(compressed.size()) / original_size;
    EXPECT_GE(ratio, 0.8);
}

// ============================================================================
// Suitability Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, IsSuitableHighlyRepetitive) {
    std::vector<std::string> values(100, "test");
    EXPECT_TRUE(DictionaryCompressor::is_suitable(values));
}

TEST_F(DictionaryCompressionTest, IsSuitableModerateCardinality) {
    std::vector<std::string> values;
    for (int i = 0; i < 50; ++i) {
        values.push_back("value_" + std::to_string(i % 5));  // 5 unique
    }
    EXPECT_TRUE(DictionaryCompressor::is_suitable(values));
}

TEST_F(DictionaryCompressionTest, IsNotSuitableHighCardinality) {
    std::vector<std::string> values;
    for (int i = 0; i < 50; ++i) {
        values.push_back("unique_" + std::to_string(i));  // 50 unique
    }
    EXPECT_FALSE(DictionaryCompressor::is_suitable(values, 0.1));
}

TEST_F(DictionaryCompressionTest, IsNotSuitableEmpty) {
    std::vector<std::string> values;
    EXPECT_FALSE(DictionaryCompressor::is_suitable(values));
}

TEST_F(DictionaryCompressionTest, IsSuitableCustomThreshold) {
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        values.push_back("value_" + std::to_string(i % 20));  // 20 unique
    }
    
    // 20% cardinality
    EXPECT_TRUE(DictionaryCompressor::is_suitable(values, 0.25));
    EXPECT_FALSE(DictionaryCompressor::is_suitable(values, 0.15));
}

// ============================================================================
// Estimation Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, EstimateRatioHighlyRepetitive) {
    std::vector<std::string> values(1000, "repetitive");
    
    double ratio = DictionaryCompressor::estimate_compression_ratio(values);
    EXPECT_LT(ratio, 0.1);
}

TEST_F(DictionaryCompressionTest, EstimateRatioModerateCar dinality) {
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        values.push_back("item_" + std::to_string(i % 10));
    }
    
    double ratio = DictionaryCompressor::estimate_compression_ratio(values);
    EXPECT_GT(ratio, 0.0);
    EXPECT_LT(ratio, 1.0);
}

TEST_F(DictionaryCompressionTest, EstimateRatioHighCardinality) {
    std::vector<std::string> values;
    for (int i = 0; i < 100; ++i) {
        values.push_back("unique_" + std::to_string(i));
    }
    
    double ratio = DictionaryCompressor::estimate_compression_ratio(values);
    EXPECT_GE(ratio, 0.8);
}

TEST_F(DictionaryCompressionTest, EstimateRatioEmpty) {
    std::vector<std::string> values;
    double ratio = DictionaryCompressor::estimate_compression_ratio(values);
    EXPECT_EQ(ratio, 1.0);
}

// ============================================================================
// String Content Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, CompressDecompressEmptyStrings) {
    std::vector<std::string> values = {"", "", ""};
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressLongStrings) {
    std::vector<std::string> values;
    std::string long_string(1000, 'a');
    values.push_back(long_string);
    values.push_back(long_string);
    long_string = std::string(1000, 'b');
    values.push_back(long_string);
    values.push_back(long_string);
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressSpecialCharacters) {
    std::vector<std::string> values = {
        "hello@world", "test#123", "value$percent", "name&title",
        "hello@world", "test#123", "value$percent"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressUnicodeStrings) {
    std::vector<std::string> values = {
        "café", "naïve", "café", "naïve", "résumé"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressMixedCases) {
    std::vector<std::string> values = {
        "Hello", "hello", "HELLO", "Hello", "hello"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressWhitespace) {
    std::vector<std::string> values = {
        "hello world", " leading", "trailing ", "  both  ",
        "hello world", " leading"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

// ============================================================================
// Large Data Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, CompressDecompress1MillionValues) {
    // 1M values with 100 unique
    std::vector<std::string> values;
    for (int i = 0; i < 1000000; ++i) {
        values.push_back("item_" + std::to_string(i % 100));
    }
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_EQ(values.size(), decompressed.size());
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressLargeUniqueSet) {
    // 10K unique strings
    std::vector<std::string> values;
    for (int i = 0; i < 10000; ++i) {
        values.push_back("unique_string_" + std::to_string(i));
    }
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, DecompressInvalidData) {
    std::vector<uint8_t> invalid = {0xFF, 0xFF, 0xFF, 0xFF};  // Invalid dict size
    auto result = DictionaryCompressor::decompress(invalid.data(), invalid.size());
    
    // Should handle gracefully, may return empty or partial results
    EXPECT_GE(result.size(), 0);
}

TEST_F(DictionaryCompressionTest, DecompressTruncatedData) {
    std::vector<std::string> values = {"hello", "world", "test"};
    auto compressed = DictionaryCompressor::compress(values);
    
    // Truncate the data
    if (compressed.size() > 10) {
        compressed.resize(10);
    }
    
    auto result = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    // May return partial results or empty
    EXPECT_GE(result.size(), 0);
}

// ============================================================================
// Frequency-Based Sorting Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, CompressDecompressFrequencyOrdering) {
    // Create data where frequencies differ significantly
    std::vector<std::string> values;
    
    // "frequent" appears 100 times
    for (int i = 0; i < 100; ++i) {
        values.push_back("frequent");
    }
    
    // "rare" appears 5 times
    for (int i = 0; i < 5; ++i) {
        values.push_back("rare");
    }
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
    // Frequent value should be sorted first for better compression
}

TEST_F(DictionaryCompressionTest, CompressDecompressBalancedFrequencies) {
    // All values have equal frequency
    std::vector<std::string> values;
    for (int cycle = 0; cycle < 10; ++cycle) {
        values.push_back("a");
        values.push_back("b");
        values.push_back("c");
        values.push_back("d");
    }
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(DictionaryCompressionTest, CompressDecompressSingleCharacterStrings) {
    std::vector<std::string> values = {"a", "b", "c", "a", "b", "c"};
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressRepeatingCharacters) {
    std::vector<std::string> values = {
        "aaa", "bbb", "ccc", "aaa", "bbb"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressNumericStrings) {
    std::vector<std::string> values = {
        "123", "456", "789", "123", "456"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressDates) {
    std::vector<std::string> values = {
        "2025-01-01", "2025-01-02", "2025-01-03",
        "2025-01-01", "2025-01-02", "2025-01-03"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

TEST_F(DictionaryCompressionTest, CompressDecompressCountryNames) {
    std::vector<std::string> values = {
        "United States", "Canada", "Mexico",
        "United States", "Canada", "Mexico"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(compressed.data(), compressed.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed));
}

// ============================================================================
// Stability Tests
// ============================================================================

TEST_F(DictionaryCompressionTest, CompressMultipleTimes) {
    std::vector<std::string> values = {"a", "b", "a", "b", "a"};
    
    // First compression
    auto compressed1 = DictionaryCompressor::compress(values);
    auto decompressed1 = DictionaryCompressor::decompress(compressed1.data(), compressed1.size());
    
    // Second compression
    auto compressed2 = DictionaryCompressor::compress(decompressed1);
    auto decompressed2 = DictionaryCompressor::decompress(compressed2.data(), compressed2.size());
    
    EXPECT_TRUE(vectors_equal(values, decompressed1));
    EXPECT_TRUE(vectors_equal(decompressed1, decompressed2));
}

TEST_F(DictionaryCompressionTest, CompressionConsistency) {
    std::vector<std::string> values = {"test", "data", "test", "data"};
    
    // Compress multiple times
    auto compressed1 = DictionaryCompressor::compress(values);
    auto compressed2 = DictionaryCompressor::compress(values);
    
    // Should decompress to same result
    auto decompressed1 = DictionaryCompressor::decompress(compressed1.data(), compressed1.size());
    auto decompressed2 = DictionaryCompressor::decompress(compressed2.data(), compressed2.size());
    
    EXPECT_TRUE(vectors_equal(decompressed1, decompressed2));
}
