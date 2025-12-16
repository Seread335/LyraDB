#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

// Optional Arrow support
#ifdef ARROW_FOUND
#include <arrow/api.h>
#endif

namespace lyradb {

/**
 * @brief LyraDB Data Type System
 * Arrow-compatible types for efficient columnar processing
 */

enum class DataType : uint8_t {
    INT32 = 0,
    INT64 = 1,
    FLOAT32 = 2,
    FLOAT64 = 3,
    STRING = 4,
    BOOL = 5,
    DATE32 = 6,
    TIMESTAMP = 7,
    DECIMAL = 8,
    NULL_TYPE = 9,
};

/**
 * @brief Type metadata and utilities
 */
class Type {
public:
    explicit Type(DataType type) : type_(type) {}
    
    #ifdef ARROW_FOUND
    static std::shared_ptr<arrow::DataType> to_arrow_type(DataType type);
    #endif
    static std::string to_string(DataType type);
    static size_t size_bytes(DataType type);
    
    DataType get_type() const { return type_; }
    
private:
    DataType type_;
};

/**
 * @brief NULL bitmap for tracking NULL values
 * Compact 8-value-per-byte representation
 */
class NullBitmap {
public:
    explicit NullBitmap(size_t capacity);
    
    void set_null(size_t idx, bool is_null);
    bool is_null(size_t idx) const;
    
    const uint8_t* data() const { return bitmap_.data(); }
    size_t byte_size() const { return bitmap_.size(); }
    
private:
    std::vector<uint8_t> bitmap_;
};

#ifdef ARROW_FOUND
/**
 * @brief Arrow Array wrapper for type safety
 */
class Array {
public:
    Array(std::shared_ptr<arrow::Array> arr) : array_(arr) {}
    
    std::shared_ptr<arrow::Array> get() const { return array_; }
    size_t length() const { return array_->length(); }
    size_t null_count() const { return array_->null_count(); }
    
private:
    std::shared_ptr<arrow::Array> array_;
};
#endif

} // namespace lyradb
