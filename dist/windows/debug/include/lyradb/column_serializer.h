#pragma once

#include "lyradb/storage_format.h"
#include <vector>
#include <memory>
#include <string>

namespace lyradb {
namespace storage {

/**
 * @brief File-based column storage writer
 * Serializes column data to .lycol format
 */
class ColumnWriter {
public:
    /**
     * @brief Create a new column file writer
     * @param filepath Path to .lycol file to write
     * @param column_id ID of this column
     * @param data_type Data type of column
     */
    ColumnWriter(const std::string& filepath, uint32_t column_id, uint8_t data_type);
    
    /**
     * @brief Write table metadata header
     */
    void write_table_metadata(const TableMetadata& metadata);
    
    /**
     * @brief Write a page of data
     * @param data Uncompressed page data
     * @param size Size of data
     * @param row_count Number of rows in this page
     * @param compression_algo Which compression algorithm to use
     */
    void write_page(
        const uint8_t* data,
        size_t size,
        uint32_t row_count,
        uint8_t compression_algo);
    
    /**
     * @brief Finalize and close file
     * Writes index and checksum
     */
    void finalize();
    
    /**
     * @brief Get file offset for current page
     */
    uint64_t current_offset() const;
    
    /**
     * @brief Get total bytes written
     */
    uint64_t total_bytes_written() const;

private:
    std::string filepath_;
    uint32_t column_id_;
    uint8_t data_type_;
    uint64_t page_count_;
    uint64_t bytes_written_;
    std::vector<PageMetadata> page_index_;
    
    /**
     * @brief Calculate CRC32 checksum
     */
    uint32_t calculate_crc32(const uint8_t* data, size_t size);
};

/**
 * @brief File-based column storage reader
 * Reads column data from .lycol format
 */
class ColumnReader {
public:
    /**
     * @brief Open an existing .lycol file
     * @param filepath Path to .lycol file to read
     */
    explicit ColumnReader(const std::string& filepath);
    
    /**
     * @brief Read table metadata header
     */
    TableMetadata read_table_metadata();
    
    /**
     * @brief Read a specific page
     * @param page_index Which page to read (0-based)
     * @return Decompressed page data
     */
    std::vector<uint8_t> read_page(uint32_t page_index);
    
    /**
     * @brief Read all pages for a column
     * @return Vector of decompressed pages
     */
    std::vector<std::vector<uint8_t>> read_all_pages();
    
    /**
     * @brief Get page metadata for specific page
     */
    PageMetadata get_page_metadata(uint32_t page_index) const;
    
    /**
     * @brief Get total number of pages
     */
    uint32_t page_count() const;
    
    /**
     * @brief Validate file integrity
     */
    bool validate();

private:
    std::string filepath_;
    TableMetadata metadata_;
    std::vector<PageMetadata> page_index_;
    bool is_valid_;
    
    /**
     * @brief Load file index
     */
    void load_index();
    
    /**
     * @brief Verify CRC32
     */
    bool verify_crc32(const uint8_t* data, size_t size, uint32_t expected_crc);
};

} // namespace storage
} // namespace lyradb
