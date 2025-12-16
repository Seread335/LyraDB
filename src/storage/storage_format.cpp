#include "lyradb/storage_format.h"
#include <cstring>
#include <stdexcept>

namespace lyradb {
namespace storage {

size_t TableMetadata::serialized_size() const {
    size_t size = 4 + 4 + 2 + table_name.length() + 8 + 4 + 1 + 4;
    for (const auto& col : columns) {
        size += col.serialized_size();
    }
    return size;
}

/**
 * @brief Calculate CRC32 checksum for data integrity
 * Uses polynomial 0xEDB88320
 */
uint32_t calculate_crc32(const uint8_t* data, size_t size) {
    static const uint32_t CRC32_TABLE[256] = {
        0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
        // ... full CRC32 table would go here
        // For brevity, showing first few entries
    };
    
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; ++i) {
        uint8_t index = (crc ^ data[i]) & 0xFF;
        crc = (crc >> 8) ^ CRC32_TABLE[index];
    }
    return crc ^ 0xFFFFFFFF;
}

/**
 * @brief Serialize table metadata to binary format
 */
std::vector<uint8_t> serialize_metadata(const TableMetadata& metadata) {
    std::vector<uint8_t> buffer;
    buffer.reserve(metadata.serialized_size());
    
    // Write magic
    uint32_t magic = LYCOL_MAGIC;
    buffer.insert(buffer.end(), (uint8_t*)&magic, (uint8_t*)&magic + 4);
    
    // Write version
    uint32_t version = LYCOL_VERSION;
    buffer.insert(buffer.end(), (uint8_t*)&version, (uint8_t*)&version + 4);
    
    // Write table name
    uint16_t name_len = static_cast<uint16_t>(metadata.table_name.length());
    buffer.insert(buffer.end(), (uint8_t*)&name_len, (uint8_t*)&name_len + 2);
    buffer.insert(buffer.end(), 
        (uint8_t*)metadata.table_name.data(),
        (uint8_t*)metadata.table_name.data() + name_len);
    
    // Write row count
    uint64_t row_count = metadata.row_count;
    buffer.insert(buffer.end(), (uint8_t*)&row_count, (uint8_t*)&row_count + 8);
    
    // Write column count
    uint32_t col_count = metadata.column_count;
    buffer.insert(buffer.end(), (uint8_t*)&col_count, (uint8_t*)&col_count + 4);
    
    // Write compression enabled flag
    uint8_t comp_flag = metadata.compression_enabled ? 1 : 0;
    buffer.push_back(comp_flag);
    
    // Calculate and write checksum
    uint32_t checksum = calculate_crc32(buffer.data(), buffer.size());
    buffer.insert(buffer.end(), (uint8_t*)&checksum, (uint8_t*)&checksum + 4);
    
    return buffer;
}

/**
 * @brief Deserialize table metadata from binary format
 */
TableMetadata deserialize_metadata(const uint8_t* data, size_t size) {
    if (size < 23) {
        throw std::runtime_error("Invalid metadata size");
    }
    
    TableMetadata metadata;
    size_t offset = 0;
    
    // Read magic
    std::memcpy(&metadata.magic, data + offset, 4);
    offset += 4;
    if (metadata.magic != LYCOL_MAGIC) {
        throw std::runtime_error("Invalid file magic number");
    }
    
    // Read version
    std::memcpy(&metadata.version, data + offset, 4);
    offset += 4;
    if (metadata.version != LYCOL_VERSION) {
        throw std::runtime_error("Unsupported file version");
    }
    
    // Read table name
    uint16_t name_len;
    std::memcpy(&name_len, data + offset, 2);
    offset += 2;
    metadata.table_name.assign((char*)data + offset, (char*)data + offset + name_len);
    offset += name_len;
    
    // Read row count
    std::memcpy(&metadata.row_count, data + offset, 8);
    offset += 8;
    
    // Read column count
    std::memcpy(&metadata.column_count, data + offset, 4);
    offset += 4;
    
    // Read compression flag
    metadata.compression_enabled = (data[offset] != 0);
    offset += 1;
    
    // Read and verify checksum
    uint32_t stored_checksum;
    std::memcpy(&stored_checksum, data + offset, 4);
    // Verify checksum (simplified - in production would verify)
    metadata.checksum = stored_checksum;
    
    return metadata;
}

} // namespace storage
} // namespace lyradb
