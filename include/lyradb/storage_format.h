#pragma once

#include <cstdint>
#include <vector>
#include <cstring>
#include <string>

namespace lyradb {
namespace storage {

/**
 * @brief .lycol File Format Specification
 * 
 * File Structure:
 * ├─ Magic Header (4 bytes): "LYCO"
 * ├─ Version (4 bytes): Format version
 * ├─ Table Metadata
 * │  ├─ Table Name Length (4 bytes)
 * │  ├─ Table Name (variable)
 * │  ├─ Column Count (4 bytes)
 * │  └─ Column Definitions
 * ├─ Page Data
 * │  ├─ Page Header
 * │  ├─ Page Metadata
 * │  └─ Compressed/Uncompressed Data
 * └─ Footer with Index
 */

// Magic number for .lycol files
constexpr uint32_t LYCOL_MAGIC = 0x4F43594C;  // "LYCO" in little-endian
constexpr uint32_t LYCOL_VERSION = 1;
constexpr uint32_t LYCOL_PAGE_SIZE = 65536;   // 64KB pages

/**
 * @brief Page Header - stored at the beginning of each page
 * 
 * Structure (48 bytes):
 * ├─ Magic (4): Page magic marker
 * ├─ Page ID (8): Unique page identifier
 * ├─ Column ID (4): Which column this page belongs to
 * ├─ Row Count (4): Number of rows in page
 * ├─ Compression Algo (1): Algorithm used (0=none, 1=RLE, 2=Dict, 3=Bitpack, 4=Delta, 5=ZSTD)
 * ├─ Compression Ratio (4): Achieved compression ratio (stored as uint32_t percentage)
 * ├─ Original Size (8): Uncompressed size in bytes
 * ├─ Compressed Size (8): Compressed size in bytes
 * ├─ CRC32 Checksum (4): Data integrity check
 * └─ Padding (0): Aligned to 48 bytes
 */
#pragma pack(push, 1)
struct PageHeader {
    static constexpr uint32_t MAGIC = 0x50474841;  // "PGHA"
    
    uint32_t magic;              // Page magic marker
    uint64_t page_id;            // Unique page identifier
    uint32_t column_id;          // Column this page belongs to
    uint32_t row_count;          // Number of rows in this page
    uint8_t  compression_algo;   // Compression algorithm (0-5)
    uint8_t  padding1;           // Alignment
    uint16_t padding2;           // Alignment
    uint32_t compression_ratio_pct;  // Compression ratio as percentage
    uint64_t original_size;      // Original uncompressed size
    uint64_t compressed_size;    // Final compressed size
    uint32_t crc32_checksum;     // CRC32 of data section
    
    // Validation
    bool is_valid() const {
        return magic == MAGIC;
    }
    
    // Calculate compressed ratio
    double get_compression_ratio() const {
        return original_size > 0 ? 
            static_cast<double>(compressed_size) / original_size : 1.0;
    }
};
#pragma pack(pop)

// Note: PageHeader is packed to minimize padding, size guaranteed to be <= 64 bytes

/**
 * @brief Column Definition - metadata about a column
 * 
 * Structure:
 * ├─ Column ID (4 bytes)
 * ├─ Data Type (1 byte)
 * ├─ Name Length (2 bytes)
 * ├─ Name (variable)
 * ├─ Null Count (4 bytes)
 * ├─ Min Value (8 bytes, for numeric types)
 * ├─ Max Value (8 bytes, for numeric types)
 * ├─ Distinct Count (4 bytes)
 * └─ Page Count (4 bytes)
 */
struct ColumnDefinition {
    uint32_t column_id;
    uint8_t  data_type;
    uint16_t name_length;
    std::vector<char> name;
    uint32_t null_count;
    int64_t  min_value;
    int64_t  max_value;
    uint32_t distinct_count;
    uint32_t page_count;
    
    size_t serialized_size() const {
        return 4 + 1 + 2 + name_length + 4 + 8 + 8 + 4 + 4;
    }
};

/**
 * @brief Table Metadata - header for entire .lycol file
 * 
 * Structure:
 * ├─ Magic (4 bytes): "LYCO"
 * ├─ Version (4 bytes)
 * ├─ Table Name Length (2 bytes)
 * ├─ Table Name (variable)
 * ├─ Row Count (8 bytes)
 * ├─ Column Count (4 bytes)
 * ├─ Compression Enabled (1 byte)
 * ├─ Checksum (4 bytes)
 * └─ Column Definitions (variable)
 */
struct TableMetadata {
    uint32_t magic;
    uint32_t version;
    std::string table_name;
    uint64_t row_count;
    uint32_t column_count;
    bool compression_enabled;
    uint32_t checksum;
    std::vector<ColumnDefinition> columns;
    
    size_t serialized_size() const;
};

/**
 * @brief Compression Statistics - per-page compression info
 */
struct CompressionStats {
    uint8_t algorithm;           // Which compression algorithm (0-5)
    double compression_ratio;    // Achieved ratio (< 1.0 is better)
    uint64_t original_bytes;     // Original size
    uint64_t compressed_bytes;   // Compressed size
    uint64_t compression_time_us;   // Time to compress (microseconds)
    uint64_t decompression_time_us; // Time to decompress
};

/**
 * @brief Page Metadata - information about pages in file
 * 
 * Used to index and locate pages efficiently
 */
struct PageMetadata {
    uint64_t page_id;
    uint32_t column_id;
    uint32_t row_count;
    uint64_t file_offset;        // Where in file this page starts
    uint64_t page_size;          // Size of page data
    CompressionStats compression;
};

} // namespace storage
} // namespace lyradb
