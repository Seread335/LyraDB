#pragma once

#include "schema.h"
#include "column.h"
#include <string>
#include <vector>
#include <memory>
#include <variant>
#include <any>

namespace lyradb {

/**
 * @brief In-memory table representation with row storage
 */
class Table {
public:
    Table(const std::string& name, const Schema& schema);
    
    // Data manipulation
    void insert_row(const std::vector<void*>& values);
    void insert_row(const std::vector<std::string>& values);  // String-based insertion
    
    /**
     * @brief Update a specific row with new values
     * @param row_index Zero-based index of the row to update
     * @param values New values for all columns in the row
     * @throws std::runtime_error if row_index is out of bounds or values size mismatch
     */
    void update_row(size_t row_index, const std::vector<std::string>& values);
    
    /**
     * @brief Delete rows by their indices
     * @param row_indices Vector of row indices to delete (0-based)
     * Indices are sorted in descending order to avoid index shifting issues
     */
    void delete_rows(const std::vector<size_t>& row_indices);
    
    void finalize();
    
    // Query operations
    std::vector<std::vector<std::string>> scan_all() const;
    std::vector<size_t> scan_with_filter(const std::string& column, 
                                         const std::string& op, 
                                         const std::string& value) const;
    std::vector<std::vector<std::string>> get_rows(const std::vector<size_t>& row_ids) const;
    
    // Accessors
    const std::string& name() const { return name_; }
    const Schema& get_schema() const;
    std::shared_ptr<Column> get_column(const std::string& name);
    std::shared_ptr<Column> get_column(size_t idx) { return columns_[idx]; }
    
    size_t row_count() const { return rows_.size(); }
    size_t column_count() const { return columns_.size(); }
    
    // Row accessors
    const std::vector<std::vector<std::string>>& get_all_rows() const { return rows_; }
    
private:
    std::string name_;
    Schema schema_;
    std::vector<std::shared_ptr<Column>> columns_;
    std::vector<std::vector<std::string>> rows_;  // In-memory row storage
    
    // Helper methods
    std::string convert_to_string(void* value, DataType type) const;
    bool matches_filter(const std::string& value, 
                       const std::string& op, 
                       const std::string& filter_value) const;
};

} // namespace lyradb
