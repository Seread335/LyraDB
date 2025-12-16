#pragma once

#include "data_types.h"
#include <string>
#include <vector>
#include <memory>

namespace lyradb {

/**
 * @brief Column definition
 */
struct ColumnDef {
    std::string name;
    DataType type;
    bool nullable = true;
    
    ColumnDef(const std::string& n, DataType t, bool null = true)
        : name(n), type(t), nullable(null) {}
};

/**
 * @brief Table schema
 */
class Schema {
public:
    Schema() = default;
    explicit Schema(const std::vector<ColumnDef>& columns);
    
    void add_column(const ColumnDef& col);
    const ColumnDef& get_column(size_t idx) const;
    const ColumnDef* find_column(const std::string& name) const;
    
    size_t num_columns() const { return columns_.size(); }
    size_t column_index(const std::string& name) const;
    
    std::string to_json() const;
    static Schema from_json(const std::string& json_str);
    
private:
    std::vector<ColumnDef> columns_;
};

} // namespace lyradb
