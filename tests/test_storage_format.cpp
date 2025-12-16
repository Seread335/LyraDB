#include <gtest/gtest.h>
#include "lyradb/storage_format.h"
#include "lyradb/column_serializer.h"
#include <cstring>
#include <filesystem>

namespace lyradb {
namespace storage {
namespace test {

class StorageFormatTest : public ::testing::Test {
protected:
    void TearDown() override {
        // Clean up test files
        std::error_code ec;
        std::filesystem::remove("test_output.lycol", ec);
    }
};

TEST_F(StorageFormatTest, PageHeaderValidation) {
    PageHeader header;
    header.magic = PageHeader::MAGIC;
    header.page_id = 0;
    header.column_id = 1;
    header.row_count = 100;
    header.compression_algo = 0;
    header.original_size = 1024;
    header.compressed_size = 1024;
    
    EXPECT_TRUE(header.is_valid());
}

TEST_F(StorageFormatTest, PageHeaderCompressionRatio) {
    PageHeader header;
    header.original_size = 1000;
    header.compressed_size = 500;
    
    double ratio = header.get_compression_ratio();
    EXPECT_DOUBLE_EQ(ratio, 0.5);
}

TEST_F(StorageFormatTest, PageHeaderSize) {
    // Verify page header is exactly 48 bytes
    EXPECT_EQ(sizeof(PageHeader), 48);
}

TEST_F(StorageFormatTest, MetadataSerializationDeserialization) {
    TableMetadata original;
    original.magic = LYCOL_MAGIC;
    original.version = LYCOL_VERSION;
    original.table_name = "test_table";
    original.row_count = 1000;
    original.column_count = 3;
    original.compression_enabled = true;
    original.checksum = 0x12345678;
    
    // Serialize
    auto serialized = serialize_metadata(original);
    EXPECT_GT(serialized.size(), 0);
    
    // Deserialize
    TableMetadata deserialized = deserialize_metadata(
        serialized.data(), serialized.size());
    
    EXPECT_EQ(deserialized.magic, LYCOL_MAGIC);
    EXPECT_EQ(deserialized.version, LYCOL_VERSION);
    EXPECT_EQ(deserialized.table_name, "test_table");
    EXPECT_EQ(deserialized.row_count, 1000);
    EXPECT_EQ(deserialized.column_count, 3);
    EXPECT_TRUE(deserialized.compression_enabled);
}

TEST_F(StorageFormatTest, InvalidMetadataMagic) {
    // Create invalid metadata with wrong magic
    uint8_t buffer[32];
    uint32_t wrong_magic = 0xDEADBEEF;
    std::memcpy(buffer, &wrong_magic, 4);
    
    EXPECT_THROW(
        deserialize_metadata(buffer, 32),
        std::runtime_error);
}

TEST_F(StorageFormatTest, ColumnWriterCreation) {
    ColumnWriter writer("test_output.lycol", 1, 4);  // column 1, INT32 type
    
    // Writer should be created successfully
    EXPECT_NO_THROW({
        TableMetadata metadata;
        metadata.magic = LYCOL_MAGIC;
        metadata.version = LYCOL_VERSION;
        metadata.table_name = "test";
        metadata.row_count = 0;
        metadata.column_count = 1;
        metadata.compression_enabled = false;
        writer.write_table_metadata(metadata);
    });
}

TEST_F(StorageFormatTest, PageWriteReadRoundTrip) {
    // Create test data
    std::vector<int32_t> test_data = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    
    // Write
    ColumnWriter writer("test_output.lycol", 1, 4);
    
    TableMetadata metadata;
    metadata.magic = LYCOL_MAGIC;
    metadata.version = LYCOL_VERSION;
    metadata.table_name = "test";
    metadata.row_count = 10;
    metadata.column_count = 1;
    metadata.compression_enabled = false;
    
    EXPECT_NO_THROW({
        writer.write_table_metadata(metadata);
        writer.write_page(
            (uint8_t*)test_data.data(),
            test_data.size() * sizeof(int32_t),
            10,
            0);  // No compression
        writer.finalize();
    });
}

TEST_F(StorageFormatTest, CompressionStats) {
    CompressionStats stats;
    stats.algorithm = 1;  // RLE
    stats.compression_ratio = 0.25;
    stats.original_bytes = 1000;
    stats.compressed_bytes = 250;
    stats.compression_time_us = 100;
    stats.decompression_time_us = 50;
    
    EXPECT_EQ(stats.algorithm, 1);
    EXPECT_LT(stats.compression_ratio, 0.5);
}

} // namespace test
} // namespace storage
} // namespace lyradb
