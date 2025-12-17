#pragma once

#include <string>
#include <vector>
#include <memory>

namespace lyradb {

// Forward declaration
class Table;
class Array;

/**
 * @brief Query execution result
 */
class QueryResult {
public:
    QueryResult() = default;
    virtual ~QueryResult() = default;
    
    virtual size_t row_count() const = 0;
    virtual size_t column_count() const = 0;
    virtual std::vector<std::string> column_names() const = 0;
    virtual std::shared_ptr<Array> get_column(size_t idx) const = 0;
};

/**
 * @brief Concrete implementation for in-memory query results
 */
class EngineQueryResult : public QueryResult {
public:
    /**
     * @brief Constructor for in-memory row result
     * @param rows Vector of rows (each row is a vector of strings)
     * @param column_names Names of columns
     */
    EngineQueryResult(const std::vector<std::vector<std::string>>& rows,
                     const std::vector<std::string>& col_names)
        : rows_(rows), column_names_(col_names), affected_rows_(0) {}
    
    /**
     * @brief Constructor for empty result
     * @param col_names Names of columns
     */
    EngineQueryResult(const std::vector<std::string>& col_names)
        : column_names_(col_names), affected_rows_(0) {}
    
    /**
     * @brief Default constructor
     */
    EngineQueryResult() : affected_rows_(0) {}
    
    virtual ~EngineQueryResult() = default;
    
    /**
     * @brief Get number of rows in result
     */
    size_t row_count() const override { return rows_.size(); }
    
    /**
     * @brief Get number of columns in result
     */
    size_t column_count() const override { return column_names_.size(); }
    
    /**
     * @brief Get column names
     */
    std::vector<std::string> column_names() const override { return column_names_; }
    
    /**
     * @brief Get a column by index (not fully implemented, returns nullptr)
     */
    std::shared_ptr<Array> get_column(size_t idx) const override { return nullptr; }
    
    /**
     * @brief Get value at specific row and column (as string)
     * @param row_idx Row index (0-based)
     * @param col_idx Column index (0-based)
     * @return Value as string, or empty string if out of bounds
     */
    std::string get_value(size_t row_idx, size_t col_idx) const {
        if (row_idx >= rows_.size() || col_idx >= column_names_.size()) {
            return "";
        }
        return rows_[row_idx][col_idx];
    }
    
    /**
     * @brief Get value at specific row and column (as int)
     * @param row_idx Row index (0-based)
     * @param col_idx Column index (0-based)
     * @return Value as int, or 0 if invalid
     */
    int get_int(size_t row_idx, size_t col_idx) const {
        try {
            return std::stoi(get_value(row_idx, col_idx));
        } catch (...) {
            return 0;
        }
    }
    
    /**
     * @brief Get value at specific row and column (as double)
     * @param row_idx Row index (0-based)
     * @param col_idx Column index (0-based)
     * @return Value as double, or 0.0 if invalid
     */
    double get_double(size_t row_idx, size_t col_idx) const {
        try {
            return std::stod(get_value(row_idx, col_idx));
        } catch (...) {
            return 0.0;
        }
    }
    
    /**
     * @brief Get value at specific row and column (as string)
     * @param row_idx Row index (0-based)
     * @param col_idx Column index (0-based)
     * @return Pointer to string value
     */
    const char* get_string(size_t row_idx, size_t col_idx) const {
        static thread_local std::string buffer;
        buffer = get_value(row_idx, col_idx);
        return buffer.c_str();
    }
    
    /**
     * @brief Get value at specific row and column (as bool)
     * @param row_idx Row index (0-based)
     * @param col_idx Column index (0-based)
     * @return Value as bool (true/false or 1/0)
     */
    bool get_bool(size_t row_idx, size_t col_idx) const {
        std::string val = get_value(row_idx, col_idx);
        if (val.empty()) return false;
        if (val == "true" || val == "1" || val == "TRUE") return true;
        return false;
    }
    
    /**
     * @brief Add a row to the result
     */
    void add_row(const std::vector<std::string>& row) {
        rows_.push_back(row);
    }
    
    /**
     * @brief Get all rows
     */
    const std::vector<std::vector<std::string>>& get_rows() const {
        return rows_;
    }
    
    /**
     * @brief Set the number of affected rows (for UPDATE/DELETE)
     */
    void set_affected_rows(int count) {
        affected_rows_ = count;
    }
    
    /**
     * @brief Get the number of affected rows
     */
    int get_affected_rows() const {
        return affected_rows_;
    }

private:
    std::vector<std::vector<std::string>> rows_;
    std::vector<std::string> column_names_;
    int affected_rows_;
};

}  // namespace lyradb
