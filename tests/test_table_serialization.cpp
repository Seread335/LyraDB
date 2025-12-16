#include <gtest/gtest.h>
#include <fstream>
#include <filesystem>
#include "lyradb/table_serializer.h"
#include "lyradb/table_format.h"
#include "lyradb/schema.h"
#include "lyradb/compression_selector.h"

namespace fs = std::filesystem;
using namespace lyradb;
using namespace lyradb::storage;

// Test fixture for table serialization tests
class TableSerializationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create test directory
        test_dir_ = "./test_tables";
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
        fs::create_directory(test_dir_);
    }

    void TearDown() override {
        // Cleanup test directory
        if (fs::exists(test_dir_)) {
            fs::remove_all(test_dir_);
        }
    }

    std::string test_dir_;

    // Helper to create a test schema
    Schema create_test_schema() {
        Schema schema;
        schema.add_column({"id", DataType::INT64});
        schema.add_column({"name", DataType::STRING});
        schema.add_column({"age", DataType::INT32});
        schema.add_column({"salary", DataType::DOUBLE});
        return schema;
    }

    // Generate test data for INT64 column
    std::vector<std::vector<uint8_t>> generate_int64_pages(size_t num_pages, 
                                                            size_t values_per_page) {
        std::vector<std::vector<uint8_t>> pages;
        
        for (size_t p = 0; p < num_pages; ++p) {
            std::vector<uint8_t> page;
            page.reserve(values_per_page * 8);
            
            for (size_t i = 0; i < values_per_page; ++i) {
                int64_t value = static_cast<int64_t>(p * values_per_page + i);
                uint8_t* ptr = reinterpret_cast<uint8_t*>(&value);
                page.insert(page.end(), ptr, ptr + 8);
            }
            
            pages.push_back(page);
        }
        
        return pages;
    }

    // Generate test data for STRING column
    std::vector<std::vector<uint8_t>> generate_string_pages(size_t num_pages,
                                                             size_t strings_per_page) {
        std::vector<std::vector<uint8_t>> pages;
        
        for (size_t p = 0; p < num_pages; ++p) {
            std::vector<uint8_t> page;
            
            for (size_t i = 0; i < strings_per_page; ++i) {
                std::string str = "Name_" + std::to_string(p * strings_per_page + i);
                
                // Store string length first (4 bytes)
                uint32_t len = str.length();
                uint8_t* len_ptr = reinterpret_cast<uint8_t*>(&len);
                page.insert(page.end(), len_ptr, len_ptr + 4);
                
                // Then the string data
                page.insert(page.end(), str.begin(), str.end());
            }
            
            pages.push_back(page);
        }
        
        return pages;
    }
};

// ============================================================================
// Test Cases
// ============================================================================

// Test 1: Table file header validation
TEST_F(TableSerializationTest, TableFileHeaderValidation) {
    TableFileHeader header;
    header.magic = LYTA_MAGIC;
    header.version = LYTA_VERSION;
    header.row_count = 1000;
    header.column_count = 4;
    header.schema_id = 1;
    header.checksum = 0;

    // Serialize and deserialize
    auto bytes = format_utils::serialize_table_header(header);
    EXPECT_EQ(bytes.size(), sizeof(TableFileHeader));

    // Update checksum
    header.checksum = format_utils::calculate_table_checksum(
        bytes.data(), bytes.size());
    bytes = format_utils::serialize_table_header(header);

    TableFileHeader deserialized = 
        format_utils::deserialize_table_header(bytes.data(), bytes.size());

    EXPECT_EQ(deserialized.magic, LYTA_MAGIC);
    EXPECT_EQ(deserialized.version, LYTA_VERSION);
    EXPECT_EQ(deserialized.row_count, 1000);
    EXPECT_EQ(deserialized.column_count, 4);
}

// Test 2: Column metadata serialization round trip
TEST_F(TableSerializationTest, ColumnMetadataSerializationRoundTrip) {
    TableColumnMetadata meta;
    meta.column_id = 0;
    meta.column_file_offset = 256;
    meta.column_file_size = 50000;
    meta.compression_algorithm = 4;  // ZSTD
    meta.page_count = 10;
    meta.compression_ratio = 45.5;
    meta.checksum = 0;

    auto bytes = format_utils::serialize_column_metadata(meta);
    EXPECT_EQ(bytes.size(), sizeof(TableColumnMetadata));

    TableColumnMetadata deserialized =
        format_utils::deserialize_column_metadata(bytes.data(), bytes.size());

    EXPECT_EQ(deserialized.column_id, 0);
    EXPECT_EQ(deserialized.column_file_offset, 256);
    EXPECT_EQ(deserialized.column_file_size, 50000);
    EXPECT_EQ(deserialized.compression_algorithm, 4);
    EXPECT_EQ(deserialized.page_count, 10);
    EXPECT_DOUBLE_EQ(deserialized.compression_ratio, 45.5);
}

// Test 3: Table writer initialization
TEST_F(TableSerializationTest, TableWriterInitialization) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/test_table.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    EXPECT_FALSE(writer.is_finalized());
    EXPECT_EQ(writer.get_statistics().total_columns, 4);
}

// Test 4: Single column write and metadata tracking
TEST_F(TableSerializationTest, SingleColumnWrite) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/test_table.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    // Generate test pages for ID column
    auto pages = generate_int64_pages(2, 100);

    CompressionStats stats;
    stats.original_size = 1600;      // 2 pages * 100 * 8 bytes
    stats.compressed_size = 800;     // Hypothetical compression
    stats.compression_ratio_pct = 50.0;

    // Write pages
    writer.write_column_pages(0, pages, 200, CompressionAlgorithm::ZSTD, stats);

    // Verify statistics
    const auto& table_stats = writer.get_statistics();
    EXPECT_EQ(table_stats.total_columns, 4);
}

// Test 5: Multiple column write with different compression
TEST_F(TableSerializationTest, MultipleColumnWriteWithDifferentCompression) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/test_table.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    // Column 0: INT64 with ZSTD
    auto pages_int = generate_int64_pages(2, 100);
    CompressionStats stats_int{1600, 800, 50.0};
    writer.write_column_pages(0, pages_int, 200, CompressionAlgorithm::ZSTD, stats_int);

    // Column 1: STRING with DICTIONARY
    auto pages_str = generate_string_pages(2, 50);
    CompressionStats stats_str{5000, 2000, 40.0};
    writer.write_column_pages(1, pages_str, 200, 
                             CompressionAlgorithm::DICTIONARY, stats_str);

    EXPECT_FALSE(writer.is_finalized());
}

// Test 6: Table finalization
TEST_F(TableSerializationTest, TableFinalization) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/test_table.lyta";

    {
        TableWriter writer(filepath, schema, test_dir_);

        auto pages = generate_int64_pages(2, 100);
        CompressionStats stats{1600, 800, 50.0};
        writer.write_column_pages(0, pages, 200, CompressionAlgorithm::ZSTD, stats);

        writer.finalize();
        EXPECT_TRUE(writer.is_finalized());
    }

    // Verify file was created
    EXPECT_TRUE(fs::exists(filepath));
}

// Test 7: Large table scenario
TEST_F(TableSerializationTest, LargeTableScenario) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/large_table.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    // Write 10 pages of 1000 values each = 10,000 rows
    auto pages = generate_int64_pages(10, 1000);
    CompressionStats stats{80000, 40000, 50.0};

    writer.write_column_pages(0, pages, 10000, CompressionAlgorithm::DELTA, stats);

    writer.finalize();

    const auto& table_stats = writer.get_statistics();
    EXPECT_EQ(table_stats.total_rows, 10000);
    EXPECT_EQ(table_stats.total_columns, 4);
}

// Test 8: Table statistics calculation
TEST_F(TableSerializationTest, TableStatisticsCalculation) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/stats_table.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    // Write multiple columns with different compression
    auto pages_1 = generate_int64_pages(2, 100);
    CompressionStats stats_1{1600, 400, 75.0};
    writer.write_column_pages(0, pages_1, 200, CompressionAlgorithm::RLE, stats_1);

    auto pages_2 = generate_int64_pages(2, 100);
    CompressionStats stats_2{1600, 800, 50.0};
    writer.write_column_pages(1, pages_2, 200, CompressionAlgorithm::ZSTD, stats_2);

    writer.finalize();

    const auto& table_stats = writer.get_statistics();
    EXPECT_GT(table_stats.overall_compression_ratio, 0.0);
    EXPECT_EQ(table_stats.total_rows, 200);
}

// Test 9: Schema integration
TEST_F(TableSerializationTest, SchemaIntegration) {
    Schema schema;
    schema.add_column({"id", DataType::INT64});
    schema.add_column({"amount", DataType::DOUBLE});
    schema.add_column({"active", DataType::BOOLEAN});

    std::string filepath = test_dir_ + "/schema_test.lyta";

    TableWriter writer(filepath, schema, test_dir_);
    EXPECT_EQ(writer.get_statistics().total_columns, 3);

    // Write data for each column type
    auto pages_int = generate_int64_pages(1, 100);
    CompressionStats stats_int{800, 400, 50.0};
    writer.write_column_pages(0, pages_int, 100, CompressionAlgorithm::DELTA, stats_int);

    writer.finalize();
    EXPECT_TRUE(fs::exists(filepath));
}

// Test 10: Multiple compression algorithms
TEST_F(TableSerializationTest, MultipleCompressionAlgorithms) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/multi_compression.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    CompressionStats stats{1000, 500, 50.0};
    auto pages = generate_int64_pages(1, 100);

    // Test each compression algorithm
    const CompressionAlgorithm algorithms[] = {
        CompressionAlgorithm::RLE,
        CompressionAlgorithm::DICTIONARY,
        CompressionAlgorithm::BITPACKING,
        CompressionAlgorithm::DELTA,
        CompressionAlgorithm::ZSTD
    };

    // For this test, we just verify the writer accepts different algorithms
    // (Actual compression is handled by ColumnWriter)
    for (int i = 0; i < 4; ++i) {
        if (i < 4) {  // Only write to first 4 columns
            writer.write_column_pages(i, pages, 100, algorithms[i], stats);
        }
    }

    writer.finalize();
    EXPECT_TRUE(writer.is_finalized());
}

// Test 11: Checksum verification
TEST_F(TableSerializationTest, ChecksumVerification) {
    TableFileHeader header;
    header.magic = LYTA_MAGIC;
    header.version = LYTA_VERSION;
    header.row_count = 1000;
    header.column_count = 4;
    header.schema_id = 1;
    header.checksum = 0;

    // Calculate correct checksum
    auto bytes = format_utils::serialize_table_header(header);
    uint32_t correct_checksum = format_utils::calculate_table_checksum(
        bytes.data(), bytes.size());
    header.checksum = correct_checksum;

    // Verify checksum
    EXPECT_TRUE(format_utils::verify_table_header_checksum(header));

    // Corrupt checksum and verify it fails
    header.checksum = correct_checksum ^ 0xFF;
    EXPECT_FALSE(format_utils::verify_table_header_checksum(header));
}

// Test 12: Error handling - invalid schema
TEST_F(TableSerializationTest, ErrorHandlingInvalidSchema) {
    Schema empty_schema;  // Empty schema

    std::string filepath = test_dir_ + "/empty_table.lyta";

    // Should still create writer but with 0 columns
    TableWriter writer(filepath, empty_schema, test_dir_);
    EXPECT_EQ(writer.get_statistics().total_columns, 0);
}

// Test 13: Mixed data types table
TEST_F(TableSerializationTest, MixedDataTypesTable) {
    Schema schema;
    schema.add_column({"id", DataType::INT64});
    schema.add_column({"name", DataType::STRING});
    schema.add_column({"salary", DataType::DOUBLE});
    schema.add_column({"active", DataType::BOOLEAN});

    std::string filepath = test_dir_ + "/mixed_types.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    // Verify all columns registered
    EXPECT_EQ(writer.get_statistics().total_columns, 4);

    writer.finalize();
    EXPECT_TRUE(fs::exists(filepath));
}

// Test 14: Edge case - single row table
TEST_F(TableSerializationTest, EdgeCaseSingleRowTable) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/single_row.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    // Single page with single value
    auto pages = generate_int64_pages(1, 1);
    CompressionStats stats{8, 8, 100.0};  // No compression

    writer.write_column_pages(0, pages, 1, CompressionAlgorithm::ZSTD, stats);
    writer.finalize();

    EXPECT_EQ(writer.get_statistics().total_rows, 1);
}

// Test 15: Performance - many pages
TEST_F(TableSerializationTest, PerformanceManyPages) {
    Schema schema = create_test_schema();
    std::string filepath = test_dir_ + "/many_pages.lyta";

    TableWriter writer(filepath, schema, test_dir_);

    // 100 pages of 100 values each = 10,000 rows
    auto pages = generate_int64_pages(100, 100);
    CompressionStats stats{800000, 400000, 50.0};

    writer.write_column_pages(0, pages, 10000, CompressionAlgorithm::DELTA, stats);
    writer.finalize();

    EXPECT_EQ(writer.get_statistics().total_rows, 10000);
}

}  // namespace

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
