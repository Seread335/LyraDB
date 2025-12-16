/**
 * @file index_executor.h
 * @brief Index Executor for Phase 6.4 - Real Index Operations
 * 
 * This module executes the optimization plans produced by SimpleQueryOptimizer.
 * It implements actual B-tree lookups, range scans, and set operations (intersection/union)
 * for AND/OR predicates.
 * 
 * Architecture:
 * - IndexExecutor: Main executor class
 * - IndexInfo: Metadata about available indexes
 * - IndexResults: Result sets from index operations
 * - Set operations: intersection (AND), union (OR)
 */

#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <cstdint>

namespace lyradb {
namespace integration {

/**
 * @brief Represents a single index with metadata
 */
struct IndexInfo {
    std::string name;           // Index name (e.g., "idx_age")
    std::string column_name;    // Column this index covers
    std::set<uint64_t> row_ids; // Set of row IDs in this index
    
    // For now, we'll use a simple mapping: value -> row_ids
    std::map<std::string, std::set<uint64_t>> value_to_rows;  // For equality lookups
};

/**
 * @brief Result of an index operation
 */
struct IndexResults {
    std::set<uint64_t> row_ids;  // Matching row IDs
    size_t rows_examined;        // Rows scanned
    double execution_time_ms;    // Execution time
    bool success;                // Whether operation succeeded
    std::string error_message;   // Error details if failed
};

/**
 * @brief Index Executor - Executes optimization plans with real index operations
 * 
 * Provides methods to:
 * - Lookup values in B-tree indexes (single equality)
 * - Scan ranges in indexes (range predicates)
 * - Compute set intersection (AND predicates)
 * - Compute set union (OR predicates)
 * - Materialize final result sets
 */
class IndexExecutor {
public:
    IndexExecutor();
    ~IndexExecutor();
    
    /**
     * @brief Register an index for execution
     * 
     * @param index_name Name of index (e.g., "idx_age")
     * @param column_name Column this index covers
     */
    void register_index(
        const std::string& index_name,
        const std::string& column_name);
    
    /**
     * @brief Add a value to an index (simulating index population)
     * 
     * @param index_name Index to add to
     * @param value Value being indexed
     * @param row_id Row ID for this value
     */
    void add_to_index(
        const std::string& index_name,
        const std::string& value,
        uint64_t row_id);
    
    /**
     * @brief Lookup a single value in an index (INDEX_SINGLE strategy)
     * 
     * @param index_name Index to search
     * @param value Value to find
     * @return Result set with matching row IDs
     */
    IndexResults lookup_value(
        const std::string& index_name,
        const std::string& value);
    
    /**
     * @brief Range scan an index (INDEX_RANGE strategy)
     * 
     * @param index_name Index to scan
     * @param op Operator: ">", "<", ">=", "<="
     * @param value Boundary value
     * @return Result set with matching row IDs
     */
    IndexResults range_scan(
        const std::string& index_name,
        const std::string& op,
        const std::string& value);
    
    /**
     * @brief Compute intersection of two result sets (AND predicate)
     * 
     * @param results1 First result set
     * @param results2 Second result set
     * @return Intersection result
     */
    IndexResults intersect(
        const IndexResults& results1,
        const IndexResults& results2);
    
    /**
     * @brief Compute union of two result sets (OR predicate)
     * 
     * @param results1 First result set
     * @param results2 Second result set
     * @return Union result
     */
    IndexResults unite(
        const IndexResults& results1,
        const IndexResults& results2);
    
    /**
     * @brief Get index statistics
     */
    std::string get_stats() const;
    
    /**
     * @brief Get size of an index (for planning)
     */
    size_t get_index_size(const std::string& index_name) const;

private:
    std::map<std::string, IndexInfo> indexes_;
    
    // Statistics
    uint64_t total_lookups_ = 0;
    uint64_t total_scans_ = 0;
    uint64_t total_intersections_ = 0;
    uint64_t total_unions_ = 0;
    double total_execution_time_ms_ = 0.0;
    
    /**
     * @brief Compare string values (for range scans)
     */
    bool compare_values(
        const std::string& val1,
        const std::string& val2,
        const std::string& op) const;
};

} // namespace integration
} // namespace lyradb
