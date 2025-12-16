#include "lyradb/schema.h"
#include <algorithm>
#include <stdexcept>
#include <sstream>

namespace lyradb {

Schema::Schema(const std::vector<ColumnDef>& columns)
    : columns_(columns) {}

void Schema::add_column(const ColumnDef& col) {
    columns_.push_back(col);
}

const ColumnDef& Schema::get_column(size_t idx) const {
    if (idx >= columns_.size()) {
        throw std::out_of_range("Column index out of range");
    }
    return columns_[idx];
}

const ColumnDef* Schema::find_column(const std::string& name) const {
    auto it = std::find_if(columns_.begin(), columns_.end(),
        [&name](const ColumnDef& col) { return col.name == name; });
    
    if (it == columns_.end()) {
        return nullptr;
    }
    return &(*it);
}

size_t Schema::column_index(const std::string& name) const {
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (columns_[i].name == name) {
            return i;
        }
    }
    throw std::runtime_error("Column not found: " + name);
}

std::string Schema::to_json() const {
    std::stringstream ss;
    ss << R"({"columns":[)";
    
    for (size_t i = 0; i < columns_.size(); ++i) {
        if (i > 0) ss << ",";
        ss << R"({"name":")" << columns_[i].name << R"(","type":")"
           << Type::to_string(columns_[i].type) << R"(","nullable":)"
           << (columns_[i].nullable ? "true" : "false") << "}";
    }
    
    ss << "]}";
    return ss.str();
}

Schema Schema::from_json(const std::string& json_str) {
    // TODO: Implement JSON parsing (use nlohmann/json or similar)
    Schema schema;
    return schema;
}

} // namespace lyradb
