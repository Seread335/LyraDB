#pragma once

#include "data_types.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace lyradb {

/**
 * @brief LyColumn Storage Format
 * Columnar storage with compression, indexing, and metadata
 */
class Column {
public:
    struct PageHeader {
        uint32_t page_size;
        uint32_t num_values;
        uint8_t compression_type;  // 0=none, 1=ZSTD, 2=RLE, 3=Dictionary, etc.
        uint8_t encoding_type;
        uint32_t data_size;
        uint32_t compressed_size;
    };
    
    struct ColumnStats {
        int64_t min_value = 0;
        int64_t max_value = 0;
        uint32_t null_count = 0;
        uint32_t distinct_count = 0;
        bool has_bloom_filter = false;
    };
    
    explicit Column(const std::string& name, DataType type, size_t initial_capacity = 4096);
    
    // Data manipulation
    void append_value(const void* value);
    void append_null();
    void finalize_page();
    
    // Getters
    const std::string& name() const { return name_; }
    DataType type() const { return type_; }
    size_t num_values() const { return num_values_; }
    size_t num_pages() const { return pages_.size(); }
    
    const ColumnStats& get_stats() const { return stats_; }
    const std::vector<uint8_t>& get_page(size_t page_idx) const;
    
    // Serialization
    std::vector<uint8_t> serialize() const;
    static Column deserialize(const std::vector<uint8_t>& data);
    
private:
    std::string name_;
    DataType type_;
    size_t num_values_ = 0;
    std::vector<uint8_t> current_page_;
    std::vector<std::vector<uint8_t>> pages_;
    std::vector<PageHeader> page_headers_;
    ColumnStats stats_;
    std::vector<uint8_t> null_bitmap_;
    
    void update_stats();
    std::vector<uint8_t> compress_page(const std::vector<uint8_t>& data);
};

} // namespace lyradb
