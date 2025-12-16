#include "lyradb/data_types.h"
#include <stdexcept>

#ifdef ARROW_FOUND
#include <arrow/type.h>
#endif

namespace lyradb {

#ifdef ARROW_FOUND
std::shared_ptr<arrow::DataType> Type::to_arrow_type(DataType type) {
    switch (type) {
        case DataType::INT32:
            return arrow::int32();
        case DataType::INT64:
            return arrow::int64();
        case DataType::FLOAT32:
            return arrow::float32();
        case DataType::FLOAT64:
            return arrow::float64();
        case DataType::STRING:
            return arrow::utf8();
        case DataType::BOOL:
            return arrow::boolean();
        case DataType::DATE32:
            return arrow::date32();
        case DataType::TIMESTAMP:
            return arrow::timestamp(arrow::TimeUnit::MICRO);
        case DataType::NULL_TYPE:
            return arrow::null();
        default:
            throw std::runtime_error("Unknown data type");
    }
}
#endif

std::string Type::to_string(DataType type) {
    switch (type) {
        case DataType::INT32:
            return "int32";
        case DataType::INT64:
            return "int64";
        case DataType::FLOAT32:
            return "float32";
        case DataType::FLOAT64:
            return "float64";
        case DataType::STRING:
            return "string";
        case DataType::BOOL:
            return "bool";
        case DataType::DATE32:
            return "date32";
        case DataType::TIMESTAMP:
            return "timestamp";
        case DataType::DECIMAL:
            return "decimal";
        case DataType::NULL_TYPE:
            return "null";
        default:
            return "unknown";
    }
}

size_t Type::size_bytes(DataType type) {
    switch (type) {
        case DataType::INT32:
        case DataType::FLOAT32:
        case DataType::DATE32:
            return 4;
        case DataType::INT64:
        case DataType::FLOAT64:
        case DataType::TIMESTAMP:
            return 8;
        case DataType::BOOL:
            return 1;
        case DataType::STRING:
        case DataType::DECIMAL:
            return 0;  // Variable size
        case DataType::NULL_TYPE:
            return 0;
        default:
            return 0;
    }
}

// NullBitmap implementation
NullBitmap::NullBitmap(size_t capacity)
    : bitmap_((capacity + 7) / 8, 0) {}

void NullBitmap::set_null(size_t idx, bool is_null) {
    size_t byte_idx = idx / 8;
    size_t bit_idx = idx % 8;
    if (is_null) {
        bitmap_[byte_idx] |= (1 << bit_idx);
    } else {
        bitmap_[byte_idx] &= ~(1 << bit_idx);
    }
}

bool NullBitmap::is_null(size_t idx) const {
    size_t byte_idx = idx / 8;
    size_t bit_idx = idx % 8;
    return (bitmap_[byte_idx] & (1 << bit_idx)) != 0;
}

} // namespace lyradb
