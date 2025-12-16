#include <gtest/gtest.h>
#include "lyradb/column_serializer.h"
#include "lyradb/compression_selector.h"
#include "lyradb/storage_format.h"
#include <cstring>
#include <random>
#include <cmath>
#include <filesystem>

namespace lyradb {
namespace storage {

class StorageCompressionIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_compression_page.lycol";
        // Clean up any existing test file
        std::filesystem::remove(test_file_);
    }
    
    void TearDown() override {
        // Clean up test file
        std::filesystem::remove(test_file_);
    }
    
    // Helper: Generate repetitive integer data (RLE should excel)
    std::vector<uint8_t> generate_rle_data(size_t size) {
        std::vector<uint8_t> data;
        data.resize(size);
        
        // Fill with repetitive patterns
        uint64_t value = 12345;
        for (size_t i = 0; i + 8 <= size; i += 8) {
            std::memcpy(data.data() + i, &value, 8);
        }
        return data;
    }
    
    // Helper: Generate random integer data
    std::vector<uint8_t> generate_random_data(size_t size) {
        std::vector<uint8_t> data;
        data.resize(size);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);
        
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }
    
    // Helper: Generate sorted integer data (Delta should excel)
    std::vector<uint8_t> generate_sorted_data(size_t size) {
        std::vector<uint8_t> data;
        data.resize(size);
        
        uint64_t value = 100;
        for (size_t i = 0; i + 8 <= size; i += 8) {
            std::memcpy(data.data() + i, &value, 8);
            value += 10;
        }
        return data;
    }
    
    // Helper: Generate bounded range data (Bitpacking should excel)
    std::vector<uint8_t> generate_bounded_data(size_t size) {
        std::vector<uint8_t> data;
        data.resize(size);
        
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 255);  // Small range [0, 255]
        
        for (auto& byte : data) {
            byte = static_cast<uint8_t>(dis(gen));
        }
        return data;
    }
    
    // Helper: Compare two data blocks
    bool data_equals(const std::vector<uint8_t>& a, const std::vector<uint8_t>& b) {
        if (a.size() != b.size()) return false;
        return std::memcmp(a.data(), b.data(), a.size()) == 0;
    }
    
    std::string test_file_;
};

// ========== Test: RLE Compression Round-Trip ==========
TEST_F(StorageCompressionIntegrationTest, RLECompression_RoundTrip) {
    auto data = generate_rle_data(4096);
    
    // Write with RLE compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 512, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::RLE));
    writer.finalize();
    
    // Read back
    ColumnReader reader(test_file_);
    auto read_data = reader.read_page(0);
    
    // Verify
    EXPECT_EQ(read_data.size(), data.size());
    EXPECT_TRUE(data_equals(read_data, data));
}

// ========== Test: Bitpacking Compression Round-Trip ==========
TEST_F(StorageCompressionIntegrationTest, BitpackingCompression_RoundTrip) {
    auto data = generate_bounded_data(4096);
    
    // Write with Bitpacking compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 512, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::BITPACKING));
    writer.finalize();
    
    // Read back
    ColumnReader reader(test_file_);
    auto read_data = reader.read_page(0);
    
    // Verify
    EXPECT_EQ(read_data.size(), data.size());
    EXPECT_TRUE(data_equals(read_data, data));
}

// ========== Test: Delta Compression Round-Trip ==========
TEST_F(StorageCompressionIntegrationTest, DeltaCompression_RoundTrip) {
    auto data = generate_sorted_data(4096);
    
    // Write with Delta compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 512, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::DELTA));
    writer.finalize();
    
    // Read back
    ColumnReader reader(test_file_);
    auto read_data = reader.read_page(0);
    
    // Verify
    EXPECT_EQ(read_data.size(), data.size());
    EXPECT_TRUE(data_equals(read_data, data));
}

// ========== Test: ZSTD Compression Round-Trip ==========
TEST_F(StorageCompressionIntegrationTest, ZSTDCompression_RoundTrip) {
    auto data = generate_random_data(4096);
    
    // Write with ZSTD compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 512, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::ZSTD));
    writer.finalize();
    
    // Read back
    ColumnReader reader(test_file_);
    auto read_data = reader.read_page(0);
    
    // Verify
    EXPECT_EQ(read_data.size(), data.size());
    EXPECT_TRUE(data_equals(read_data, data));
}

// ========== Test: Uncompressed Data ==========
TEST_F(StorageCompressionIntegrationTest, UncompressedData_RoundTrip) {
    auto data = generate_random_data(4096);
    
    // Write without compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 0;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 512, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::UNCOMPRESSED));
    writer.finalize();
    
    // Read back
    ColumnReader reader(test_file_);
    auto read_data = reader.read_page(0);
    
    // Verify
    EXPECT_EQ(read_data.size(), data.size());
    EXPECT_TRUE(data_equals(read_data, data));
}

// ========== Test: Multiple Pages with Different Compression ==========
TEST_F(StorageCompressionIntegrationTest, MultiplePages_DifferentCompression) {
    auto rle_data = generate_rle_data(2048);
    auto random_data = generate_random_data(2048);
    auto sorted_data = generate_sorted_data(2048);
    
    // Write multiple pages
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 1536;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(rle_data.data(), rle_data.size(), 256, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::RLE));
    writer.write_page(random_data.data(), random_data.size(), 256, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::ZSTD));
    writer.write_page(sorted_data.data(), sorted_data.size(), 256, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::DELTA));
    writer.finalize();
    
    // Read back all pages
    ColumnReader reader(test_file_);
    EXPECT_EQ(reader.page_count(), 3);
    
    auto page0 = reader.read_page(0);
    auto page1 = reader.read_page(1);
    auto page2 = reader.read_page(2);
    
    // Verify each page
    EXPECT_TRUE(data_equals(page0, rle_data));
    EXPECT_TRUE(data_equals(page1, random_data));
    EXPECT_TRUE(data_equals(page2, sorted_data));
}

// ========== Test: Compression Ratio Tracking ==========
TEST_F(StorageCompressionIntegrationTest, CompressionRatioTracking) {
    auto data = generate_rle_data(4096);
    
    // Write with RLE compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 512, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::RLE));
    writer.finalize();
    
    // Read back and verify compression stats
    ColumnReader reader(test_file_);
    auto meta_read = reader.read_table_metadata();
    auto page_meta = reader.get_page_metadata(0);
    
    // RLE should compress repetitive data significantly
    EXPECT_LE(page_meta.compression.compression_ratio, 1.0);
    EXPECT_GT(page_meta.compression.original_bytes, 0);
    EXPECT_GT(page_meta.compression.compressed_bytes, 0);
}

// ========== Test: Large Data Compression ==========
TEST_F(StorageCompressionIntegrationTest, LargeDataCompression) {
    // Create large dataset (1 MB)
    auto data = generate_rle_data(1024 * 1024);
    
    // Write with compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 131072;  // 1M / 8 bytes
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 131072, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::RLE));
    writer.finalize();
    
    // Read back
    ColumnReader reader(test_file_);
    auto read_data = reader.read_page(0);
    
    // Verify
    EXPECT_EQ(read_data.size(), data.size());
    EXPECT_TRUE(data_equals(read_data, data));
}

// ========== Test: Read All Pages ==========
TEST_F(StorageCompressionIntegrationTest, ReadAllPages_Compression) {
    auto data1 = generate_rle_data(2048);
    auto data2 = generate_random_data(2048);
    
    // Write multiple pages
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data1.data(), data1.size(), 256, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::RLE));
    writer.write_page(data2.data(), data2.size(), 256, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::ZSTD));
    writer.finalize();
    
    // Read all pages
    ColumnReader reader(test_file_);
    auto all_pages = reader.read_all_pages();
    
    // Verify
    EXPECT_EQ(all_pages.size(), 2);
    EXPECT_TRUE(data_equals(all_pages[0], data1));
    EXPECT_TRUE(data_equals(all_pages[1], data2));
}

// ========== Test: Validate All Pages ==========
TEST_F(StorageCompressionIntegrationTest, ValidateAllPages_Compression) {
    auto data = generate_random_data(4096);
    
    // Write page
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 512;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 512, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::ZSTD));
    writer.finalize();
    
    // Validate
    ColumnReader reader(test_file_);
    EXPECT_TRUE(reader.validate());
}

// ========== Test: Small Data Compression ==========
TEST_F(StorageCompressionIntegrationTest, SmallDataCompression) {
    // Test with small data (64 bytes)
    std::vector<uint8_t> data;
    data.resize(64);
    std::memset(data.data(), 42, 64);
    
    // Write with RLE compression
    ColumnWriter writer(test_file_, 1, 2);
    TableMetadata meta;
    meta.magic = TableMetadata::MAGIC;
    meta.version = 1;
    meta.table_name = "test";
    meta.row_count = 8;
    meta.column_count = 1;
    meta.enable_compression = 1;
    
    writer.write_table_metadata(meta);
    writer.write_page(data.data(), data.size(), 8, 
                     static_cast<uint8_t>(compression::CompressionAlgorithm::RLE));
    writer.finalize();
    
    // Read back
    ColumnReader reader(test_file_);
    auto read_data = reader.read_page(0);
    
    // Verify
    EXPECT_EQ(read_data.size(), data.size());
    EXPECT_TRUE(data_equals(read_data, data));
}

} // namespace storage
} // namespace lyradb

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
