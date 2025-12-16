#include "lyradb/table_format.h"
#include <cstring>
#include <stdexcept>

namespace lyradb {
namespace storage {
namespace format_utils {

// CRC32 calculation (using same implementation as storage_format.cpp)
static uint32_t crc32_table[256];
static bool crc32_initialized = false;

static void init_crc32_table() {
    if (crc32_initialized) return;
    
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int j = 0; j < 8; j++) {
            if (crc & 1) {
                crc = (crc >> 1) ^ 0xEDB88320;
            } else {
                crc >>= 1;
            }
        }
        crc32_table[i] = crc;
    }
    crc32_initialized = true;
}

static uint32_t compute_crc32(const uint8_t* data, size_t size) {
    init_crc32_table();
    
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc = (crc >> 8) ^ crc32_table[(crc ^ data[i]) & 0xFF];
    }
    return crc ^ 0xFFFFFFFF;
}

// Serialize table header
std::vector<uint8_t> serialize_table_header(const TableFileHeader& header) {
    std::vector<uint8_t> buffer(sizeof(TableFileHeader));
    uint8_t* ptr = buffer.data();
    
    // Serialize each field (little-endian)
    std::memcpy(ptr, &header.magic, 4);
    ptr += 4;
    std::memcpy(ptr, &header.version, 4);
    ptr += 4;
    std::memcpy(ptr, &header.row_count, 8);
    ptr += 8;
    std::memcpy(ptr, &header.column_count, 4);
    ptr += 4;
    std::memcpy(ptr, &header.schema_id, 4);
    ptr += 4;
    std::memcpy(ptr, &header.checksum, 4);
    ptr += 4;
    
    return buffer;
}

// Deserialize table header
TableFileHeader deserialize_table_header(const uint8_t* data, size_t size) {
    if (size < sizeof(TableFileHeader)) {
        throw std::invalid_argument("Insufficient data for table header");
    }
    
    TableFileHeader header;
    const uint8_t* ptr = data;
    
    std::memcpy(&header.magic, ptr, 4);
    ptr += 4;
    std::memcpy(&header.version, ptr, 4);
    ptr += 4;
    std::memcpy(&header.row_count, ptr, 8);
    ptr += 8;
    std::memcpy(&header.column_count, ptr, 4);
    ptr += 4;
    std::memcpy(&header.schema_id, ptr, 4);
    ptr += 4;
    std::memcpy(&header.checksum, ptr, 4);
    ptr += 4;
    
    if (header.magic != LYTA_MAGIC) {
        throw std::invalid_argument("Invalid table file magic number");
    }
    
    if (header.version != LYTA_VERSION) {
        throw std::invalid_argument("Unsupported table file version");
    }
    
    return header;
}

// Serialize column metadata
std::vector<uint8_t> serialize_column_metadata(const TableColumnMetadata& meta) {
    std::vector<uint8_t> buffer(sizeof(TableColumnMetadata));
    uint8_t* ptr = buffer.data();
    
    std::memcpy(ptr, &meta.column_id, 4);
    ptr += 4;
    std::memcpy(ptr, &meta.column_file_offset, 8);
    ptr += 8;
    std::memcpy(ptr, &meta.column_file_size, 8);
    ptr += 8;
    std::memcpy(ptr, &meta.compression_algorithm, 1);
    ptr += 1;
    std::memcpy(ptr, &meta.padding1, 1);
    ptr += 1;
    std::memcpy(ptr, &meta.padding2, 2);
    ptr += 2;
    std::memcpy(ptr, &meta.page_count, 4);
    ptr += 4;
    std::memcpy(ptr, &meta.compression_ratio, 8);
    ptr += 8;
    std::memcpy(ptr, &meta.checksum, 4);
    ptr += 4;
    
    return buffer;
}

// Deserialize column metadata
TableColumnMetadata deserialize_column_metadata(const uint8_t* data, size_t size) {
    if (size < sizeof(TableColumnMetadata)) {
        throw std::invalid_argument("Insufficient data for column metadata");
    }
    
    TableColumnMetadata meta;
    const uint8_t* ptr = data;
    
    std::memcpy(&meta.column_id, ptr, 4);
    ptr += 4;
    std::memcpy(&meta.column_file_offset, ptr, 8);
    ptr += 8;
    std::memcpy(&meta.column_file_size, ptr, 8);
    ptr += 8;
    std::memcpy(&meta.compression_algorithm, ptr, 1);
    ptr += 1;
    std::memcpy(&meta.padding1, ptr, 1);
    ptr += 1;
    std::memcpy(&meta.padding2, ptr, 2);
    ptr += 2;
    std::memcpy(&meta.page_count, ptr, 4);
    ptr += 4;
    std::memcpy(&meta.compression_ratio, ptr, 8);
    ptr += 8;
    std::memcpy(&meta.checksum, ptr, 4);
    ptr += 4;
    
    return meta;
}

// Serialize table statistics
std::vector<uint8_t> serialize_table_statistics(const TableStatistics& stats) {
    // Calculate size needed
    // Header: 8 (rows) + 4 (cols) + 8 (uncomp) + 8 (comp) + 8 (ratio) + 8 (ts)
    // String: 4 (len) + name.length() + 4 (version)
    // Per-column: 4 (id) + 8 (uncomp) + 8 (comp) + 8 (ratio) + 4 (pages) + 4 (alg_len) + alg_str + 4 (nulls) + 8 (avg) + 8 (min) + 8 (max)
    
    std::vector<uint8_t> buffer;
    buffer.reserve(512 + stats.column_stats.size() * 128);
    
    // Serialize header
    uint8_t temp[8];
    
    // total_rows (8B)
    std::memcpy(temp, &stats.total_rows, 8);
    buffer.insert(buffer.end(), temp, temp + 8);
    
    // total_columns (4B)
    uint32_t cols = stats.total_columns;
    std::memcpy(temp, &cols, 4);
    buffer.insert(buffer.end(), temp, temp + 4);
    
    // uncompressed_bytes (8B)
    std::memcpy(temp, &stats.uncompressed_bytes, 8);
    buffer.insert(buffer.end(), temp, temp + 8);
    
    // compressed_bytes (8B)
    std::memcpy(temp, &stats.compressed_bytes, 8);
    buffer.insert(buffer.end(), temp, temp + 8);
    
    // overall_compression_ratio (8B)
    std::memcpy(temp, &stats.overall_compression_ratio, 8);
    buffer.insert(buffer.end(), temp, temp + 8);
    
    // timestamp_created (8B)
    std::memcpy(temp, &stats.timestamp_created, 8);
    buffer.insert(buffer.end(), temp, temp + 8);
    
    // table_name (4B len + string)
    uint32_t name_len = stats.table_name.length();
    std::memcpy(temp, &name_len, 4);
    buffer.insert(buffer.end(), temp, temp + 4);
    buffer.insert(buffer.end(), stats.table_name.begin(), stats.table_name.end());
    
    // table_version (4B)
    std::memcpy(temp, &stats.table_version, 4);
    buffer.insert(buffer.end(), temp, temp + 4);
    
    // column_stats count (4B)
    uint32_t col_count = stats.column_stats.size();
    std::memcpy(temp, &col_count, 4);
    buffer.insert(buffer.end(), temp, temp + 4);
    
    // Serialize each column statistic
    for (const auto& col_stat : stats.column_stats) {
        // column_id (4B)
        std::memcpy(temp, &col_stat.column_id, 4);
        buffer.insert(buffer.end(), temp, temp + 4);
        
        // uncompressed_bytes (8B)
        std::memcpy(temp, &col_stat.uncompressed_bytes, 8);
        buffer.insert(buffer.end(), temp, temp + 8);
        
        // compressed_bytes (8B)
        std::memcpy(temp, &col_stat.compressed_bytes, 8);
        buffer.insert(buffer.end(), temp, temp + 8);
        
        // compression_ratio (8B)
        std::memcpy(temp, &col_stat.compression_ratio, 8);
        buffer.insert(buffer.end(), temp, temp + 8);
        
        // page_count (4B)
        std::memcpy(temp, &col_stat.page_count, 4);
        buffer.insert(buffer.end(), temp, temp + 4);
        
        // compression_algorithm (4B len + string)
        uint32_t alg_len = col_stat.compression_algorithm.length();
        std::memcpy(temp, &alg_len, 4);
        buffer.insert(buffer.end(), temp, temp + 4);
        buffer.insert(buffer.end(), col_stat.compression_algorithm.begin(), 
                      col_stat.compression_algorithm.end());
        
        // null_count (4B)
        std::memcpy(temp, &col_stat.null_count, 4);
        buffer.insert(buffer.end(), temp, temp + 4);
        
        // avg_value (8B)
        std::memcpy(temp, &col_stat.avg_value, 8);
        buffer.insert(buffer.end(), temp, temp + 8);
        
        // min_value (8B)
        std::memcpy(temp, &col_stat.min_value, 8);
        buffer.insert(buffer.end(), temp, temp + 8);
        
        // max_value (8B)
        std::memcpy(temp, &col_stat.max_value, 8);
        buffer.insert(buffer.end(), temp, temp + 8);
    }
    
    return buffer;
}

// Deserialize table statistics (stub)
TableStatistics deserialize_table_statistics(const uint8_t* data, size_t size) {
    if (size < 44) {
        throw std::invalid_argument("Insufficient data for table statistics");
    }
    
    TableStatistics stats;
    const uint8_t* ptr = data;
    
    std::memcpy(&stats.total_rows, ptr, 8);
    ptr += 8;
    std::memcpy(&stats.total_columns, ptr, 4);
    ptr += 4;
    std::memcpy(&stats.uncompressed_bytes, ptr, 8);
    ptr += 8;
    std::memcpy(&stats.compressed_bytes, ptr, 8);
    ptr += 8;
    std::memcpy(&stats.overall_compression_ratio, ptr, 8);
    ptr += 8;
    std::memcpy(&stats.timestamp_created, ptr, 8);
    ptr += 8;
    
    return stats;
}

// Calculate table checksum
uint32_t calculate_table_checksum(const uint8_t* data, size_t size) {
    return compute_crc32(data, size);
}

// Verify table header checksum
bool verify_table_header_checksum(const TableFileHeader& header) {
    // Create a copy with checksum field zeroed for verification
    TableFileHeader temp = header;
    temp.checksum = 0;
    
    std::vector<uint8_t> serialized = serialize_table_header(temp);
    uint32_t computed = compute_crc32(serialized.data(), serialized.size());
    
    return computed == header.checksum;
}

// Verify column metadata checksum
bool verify_column_metadata_checksum(const TableColumnMetadata& meta) {
    // Create a copy with checksum field zeroed for verification
    TableColumnMetadata temp = meta;
    temp.checksum = 0;
    
    std::vector<uint8_t> serialized = serialize_column_metadata(temp);
    uint32_t computed = compute_crc32(serialized.data(), serialized.size());
    
    return computed == meta.checksum;
}

}  // namespace format_utils
}  // namespace storage
}  // namespace lyradb
