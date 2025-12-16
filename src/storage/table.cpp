#include "lyradb/table.h"
#include "lyradb/data_types.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>
#include <cstring>
#include <cmath>

namespace lyradb {

Table::Table(const std::string& name, const Schema& schema)
    : name_(name), schema_(schema) {
    // Initialize columns
    for (size_t i = 0; i < schema_.num_columns(); ++i) {
        const auto& col_def = schema_.get_column(i);
        columns_.push_back(std::make_shared<Column>(col_def.name, col_def.type));
    }
}

std::string Table::convert_to_string(void* value, DataType type) const {
    if (value == nullptr) return "";
    
    std::stringstream ss;
    switch (type) {
        case DataType::INT32: {
            int val = *static_cast<int*>(value);
            ss << val;
            break;
        }
        case DataType::INT64: {
            int64_t val = *static_cast<int64_t*>(value);
            ss << val;
            break;
        }
        case DataType::FLOAT32: {
            float val = *static_cast<float*>(value);
            ss << val;
            break;
        }
        case DataType::FLOAT64: {
            double val = *static_cast<double*>(value);
            ss << val;
            break;
        }
        case DataType::STRING: {
            const char* val = static_cast<const char*>(value);
            ss << val;
            break;
        }
        case DataType::BOOL: {
            bool val = *static_cast<bool*>(value);
            ss << (val ? "true" : "false");
            break;
        }
        default:
            ss << "NULL";
    }
    return ss.str();
}

void Table::insert_row(const std::vector<void*>& values) {
    if (values.size() != schema_.num_columns()) {
        throw std::runtime_error("Row size mismatch: expected " + 
                                 std::to_string(schema_.num_columns()) + 
                                 ", got " + std::to_string(values.size()));
    }
    
    // Convert row to string format and store
    std::vector<std::string> string_row;
    for (size_t i = 0; i < values.size(); ++i) {
        const auto& col_def = schema_.get_column(i);
        string_row.push_back(convert_to_string(values[i], col_def.type));
    }
    rows_.push_back(string_row);
    
    // Also append to column-oriented storage
    for (size_t i = 0; i < values.size(); ++i) {
        if (values[i] == nullptr) {
            columns_[i]->append_null();
        } else {
            columns_[i]->append_value(values[i]);
        }
    }
}

void Table::insert_row(const std::vector<std::string>& values) {
    if (values.size() != schema_.num_columns()) {
        throw std::runtime_error("Row size mismatch");
    }
    
    rows_.push_back(values);
}

std::vector<std::vector<std::string>> Table::scan_all() const {
    return rows_;
}

bool Table::matches_filter(const std::string& value, 
                          const std::string& op, 
                          const std::string& filter_value) const {
    if (value.empty() && filter_value != "NULL") return false;
    
    if (op == "=") {
        return value == filter_value;
    } else if (op == "!=") {
        return value != filter_value;
    } else if (op == "<") {
        try {
            return std::stod(value) < std::stod(filter_value);
        } catch (...) {
            return value < filter_value;
        }
    } else if (op == "<=") {
        try {
            return std::stod(value) <= std::stod(filter_value);
        } catch (...) {
            return value <= filter_value;
        }
    } else if (op == ">") {
        try {
            return std::stod(value) > std::stod(filter_value);
        } catch (...) {
            return value > filter_value;
        }
    } else if (op == ">=") {
        try {
            return std::stod(value) >= std::stod(filter_value);
        } catch (...) {
            return value >= filter_value;
        }
    } else if (op == "LIKE") {
        return value.find(filter_value) != std::string::npos;
    }
    
    return false;
}

std::vector<size_t> Table::scan_with_filter(const std::string& column,
                                            const std::string& op,
                                            const std::string& value) const {
    std::vector<size_t> result;
    
    size_t col_idx = schema_.column_index(column);
    
    for (size_t i = 0; i < rows_.size(); ++i) {
        if (matches_filter(rows_[i][col_idx], op, value)) {
            result.push_back(i);
        }
    }
    
    return result;
}

std::vector<std::vector<std::string>> Table::get_rows(const std::vector<size_t>& row_ids) const {
    std::vector<std::vector<std::string>> result;
    for (size_t id : row_ids) {
        if (id < rows_.size()) {
            result.push_back(rows_[id]);
        }
    }
    return result;
}

std::shared_ptr<Column> Table::get_column(const std::string& name) {
    auto col_def = schema_.find_column(name);
    if (!col_def) {
        throw std::runtime_error("Column not found: " + name);
    }
    
    size_t col_idx = schema_.column_index(name);
    return columns_[col_idx];
}

const Schema& Table::get_schema() const {
    return schema_;
}

void Table::update_row(size_t row_index, const std::vector<std::string>& values) {
    if (row_index >= rows_.size()) {
        throw std::runtime_error("Row index out of bounds: " + std::to_string(row_index));
    }
    
    if (values.size() != schema_.num_columns()) {
        throw std::runtime_error("Row size mismatch: expected " + 
                                 std::to_string(schema_.num_columns()) + 
                                 ", got " + std::to_string(values.size()));
    }
    
    rows_[row_index] = values;
}

void Table::delete_rows(const std::vector<size_t>& row_indices) {
    if (row_indices.empty()) {
        return;
    }
    
    // Sort indices in descending order to delete from end to start
    // This prevents index shifting issues
    std::vector<size_t> sorted_indices = row_indices;
    std::sort(sorted_indices.rbegin(), sorted_indices.rend());
    
    // Remove duplicates
    sorted_indices.erase(std::unique(sorted_indices.begin(), sorted_indices.end()),
                         sorted_indices.end());
    
    // Delete rows
    for (size_t idx : sorted_indices) {
        if (idx < rows_.size()) {
            rows_.erase(rows_.begin() + idx);
        }
    }
}

void Table::finalize() {
    for (auto& col : columns_) {
        col->finalize_page();
    }
}

}  // namespace lyradb
