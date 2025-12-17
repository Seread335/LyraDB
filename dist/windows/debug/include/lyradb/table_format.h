#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "storage_format.h"

namespace lyradb {
namespace storage {

// Table file format constants
constexpr uint32_t LYTA_MAGIC = 0x4154594C;  // "LYTA" in little-endian
constexpr uint32_t LYTA_VERSION = 1;

// Table file header (32 bytes)
struct TableFileHeader {
    uint32_t magic;           // 4B - "LYTA"
    uint32_t version;         // 4B - Format version
    uint64_t row_count;       // 8B - Total rows in table
    uint32_t column_count;    // 4B - Number of columns
    uint32_t schema_id;       // 4B - Schema version ID
    uint32_t checksum;        // 4B - Header CRC32
};

// Per-column metadata (48 bytes)
struct TableColumnMetadata {
    uint32_t column_id;              // 4B - Column identifier
    uint64_t column_file_offset;     // 8B - Offset to .lycol data
    uint64_t column_file_size;       // 8B - Size of .lycol file
    uint8_t compression_algorithm;   // 1B - CompressionAlgorithm enum
    uint8_t padding1;                // 1B - Alignment padding
    uint16_t padding2;               // 2B - Alignment padding
    uint32_t page_count;             // 4B - Number of pages
    double compression_ratio;        // 8B - Compression ratio (%)
    uint32_t checksum;               // 4B - Column CRC32
};

// Per-column statistics
struct ColumnStatistics {
    uint32_t column_id;
    uint64_t uncompressed_bytes;
    uint64_t compressed_bytes;
    double compression_ratio;
    uint32_t page_count;
    std::string compression_algorithm;
    uint32_t null_count;
    double avg_value;
    uint64_t min_value;
    uint64_t max_value;
};

// Table-level statistics
struct TableStatistics {
    uint64_t total_rows;
    uint32_t total_columns;
    uint64_t uncompressed_bytes;
    uint64_t compressed_bytes;
    double overall_compression_ratio;
    std::vector<ColumnStatistics> column_stats;
    uint64_t timestamp_created;        // Unix timestamp
    std::string table_name;
    uint32_t table_version;
};

// Table manifest (loaded from .lyta file)
struct TableManifest {
    TableFileHeader header;
    std::vector<TableColumnMetadata> column_metadata;
    TableStatistics statistics;
    bool valid;
};

// Serialization utilities
namespace format_utils {

// Serialize table header to binary
std::vector<uint8_t> serialize_table_header(
    const TableFileHeader& header);

// Deserialize table header from binary
TableFileHeader deserialize_table_header(
    const uint8_t* data, size_t size);

// Serialize column metadata to binary
std::vector<uint8_t> serialize_column_metadata(
    const TableColumnMetadata& meta);

// Deserialize column metadata from binary
TableColumnMetadata deserialize_column_metadata(
    const uint8_t* data, size_t size);

// Serialize table statistics to binary
std::vector<uint8_t> serialize_table_statistics(
    const TableStatistics& stats);

// Deserialize table statistics from binary
TableStatistics deserialize_table_statistics(
    const uint8_t* data, size_t size);

// Calculate CRC32 for table structures
uint32_t calculate_table_checksum(const uint8_t* data, size_t size);

// Verify table header checksum
bool verify_table_header_checksum(const TableFileHeader& header);

// Verify column metadata checksum
bool verify_column_metadata_checksum(const TableColumnMetadata& meta);

}  // namespace format_utils

}  // namespace storage
}  // namespace lyradb
