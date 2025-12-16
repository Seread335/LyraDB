#include <gtest/gtest.h>
#include "lyradb/dict_compressor.h"

namespace lyradb {
namespace compression {
namespace test {

TEST(DictionaryCompressorTest, CompressStrings) {
    std::vector<std::string> values = {
        "apple", "banana", "apple", "cherry", "banana", "apple"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    
    // Should create a dictionary and compress
    EXPECT_GT(compressed.size(), 0);
}

TEST(DictionaryCompressorTest, DecompressStrings) {
    std::vector<std::string> values = {
        "cat", "dog", "cat", "bird", "dog", "cat"
    };
    
    auto compressed = DictionaryCompressor::compress(values);
    auto decompressed = DictionaryCompressor::decompress(
        compressed.data(),
        compressed.size());
    
    EXPECT_EQ(decompressed.size(), values.size());
    for (size_t i = 0; i < values.size(); ++i) {
        EXPECT_EQ(decompressed[i], values[i]);
    }
}

TEST(DictionaryCompressorTest, IsSuitable) {
    // High cardinality - not suitable
    std::vector<std::string> unique_values;
    for (int i = 0; i < 100; ++i) {
        unique_values.push_back(std::string("val_") + std::to_string(i));
    }
    
    EXPECT_FALSE(DictionaryCompressor::is_suitable(unique_values, 0.1));
    
    // Low cardinality - suitable
    std::vector<std::string> repeated_values;
    for (int i = 0; i < 100; ++i) {
        repeated_values.push_back(i % 3 == 0 ? "a" : (i % 3 == 1 ? "b" : "c"));
    }
    
    EXPECT_TRUE(DictionaryCompressor::is_suitable(repeated_values, 0.1));
}

} // namespace test
} // namespace compression
} // namespace lyradb
