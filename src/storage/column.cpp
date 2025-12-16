#include "lyradb/column.h"
#include <cstring>
#include <algorithm>
#include <stdexcept>

namespace lyradb {

Column::Column(const std::string& name, DataType type, size_t initial_capacity)
    : name_(name), type_(type) {
    current_page_.reserve(65536);  // 64 KB page size
}

void Column::append_value(const void* value) {
    size_t value_size = Type::size_bytes(type_);
    if (value_size > 0 && value != nullptr) {
        const uint8_t* bytes = static_cast<const uint8_t*>(value);
        current_page_.insert(current_page_.end(), bytes, bytes + value_size);
    }
    // Mark as not null - for now, skip bitmap update
    num_values_++;
}

void Column::append_null() {
    size_t value_size = Type::size_bytes(type_);
    if (value_size > 0) {
        std::vector<uint8_t> zeros(value_size, 0);
        current_page_.insert(current_page_.end(), zeros.begin(), zeros.end());
    }
    // Mark as null - for now, skip bitmap update
    num_values_++;
}

void Column::finalize_page() {
    if (current_page_.empty()) {
        return;
    }
    
    PageHeader header;
    header.page_size = current_page_.size();
    header.num_values = num_values_;
    header.compression_type = 0;  // No compression for now
    header.encoding_type = 0;
    header.data_size = current_page_.size();
    header.compressed_size = current_page_.size();
    
    pages_.push_back(current_page_);
    page_headers_.push_back(header);
    current_page_.clear();
    
    update_stats();
}

const std::vector<uint8_t>& Column::get_page(size_t page_idx) const {
    if (page_idx >= pages_.size()) {
        throw std::out_of_range("Page index out of range");
    }
    return pages_[page_idx];
}

void Column::update_stats() {
    // TODO: Implement statistics calculation
    // - min/max values
    // - null count
    // - distinct count
    // - bloom filter generation
}

std::vector<uint8_t> Column::compress_page(const std::vector<uint8_t>& data) {
    // TODO: Implement compression with ZSTD, RLE, Dictionary encoding
    return data;
}

std::vector<uint8_t> Column::serialize() const {
    // TODO: Implement serialization to .lycol format
    std::vector<uint8_t> result;
    return result;
}

Column Column::deserialize(const std::vector<uint8_t>& data) {
    // TODO: Implement deserialization from .lycol format
    Column col("", DataType::INT32);
    return col;
}

} // namespace lyradb
