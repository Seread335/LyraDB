#pragma once

#include <memory>
#include <vector>
#include <string>
#include <cstdint>
#include <functional>

namespace lyradb {

// Forward declarations
class QueryResult;
class QueryPlan;
class Database;
class Table;
namespace plan {
    class PlanNode;
}

/**
 * @brief Query Execution Engine
 * 
 * Executes optimized query plans with vectorized batch processing.
 * Supports:
 * - Vectorized operators (batch processing of 1K-4K rows)
 * - Multiple join algorithms
 * - Aggregate functions (SUM, COUNT, AVG, MIN, MAX)
 * - SIMD-optimized filters
 */
class QueryExecutor {
public:
    /**
     * @brief Create a new query executor
     * @param database Reference to database for table access
     */
    explicit QueryExecutor(Database* database = nullptr);
    
    ~QueryExecutor();
    
    /**
     * @brief Execute an optimized query plan
     * @param plan The optimized query plan to execute
     * @return QueryResult with rows and metadata
     */
    void execute(const QueryPlan& plan);
    
    /**
     * @brief Set batch size for vectorized processing
     * @param size Number of rows per batch (default: 1024)
     */
    void set_batch_size(size_t size);
    
    /**
     * @brief Enable/disable SIMD optimizations
     * @param enable True to enable SIMD acceleration
     */
    void set_simd_enabled(bool enable);
    
    /**
     * @brief Get execution statistics
     * @return String with performance metrics
     */
    std::string get_stats() const;

private:
    size_t batch_size_;           // Vectorized batch size
    bool simd_enabled_;           // SIMD acceleration flag
    uint64_t rows_processed_;     // Total rows processed
    uint64_t batches_processed_;  // Total batches processed
    Database* database_;          // Reference to database for table access
    
    // Execution context
    struct ExecutionContext {
        std::vector<std::vector<uint8_t>> data;  // Current batch data
        size_t row_count;                        // Rows in current batch
        std::unordered_map<std::string, size_t> column_index;  // Column name -> index
    };
    
    /**
     * @brief Execute a plan node and return batch data
     * @param node The plan node to execute
     * @param ctx Execution context for data flow
     * @return Number of rows produced
     */
    size_t execute_node(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief Execute TableScan node (read from table)
     */
    size_t execute_scan(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief Execute Filter node (apply WHERE clause)
     */
    size_t execute_filter(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief Execute Project node (SELECT columns)
     */
    size_t execute_project(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief Execute Join node
     */
    size_t execute_join(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief Execute Aggregate node (GROUP BY, SUM, COUNT, etc.)
     */
    size_t execute_aggregate(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief Execute Sort node (ORDER BY)
     */
    size_t execute_sort(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief Execute Limit node (LIMIT/OFFSET)
     */
    size_t execute_limit(const plan::PlanNode* node, ExecutionContext& ctx);
    
    /**
     * @brief PHASE 4.3: Execute indexed scan using B-tree index
     * @param index_name Name of the index to use
     * @param column Column being indexed
     * @param predicate Predicate (e.g., "> 100")
     * @param ctx Execution context
     * @return Number of matching rows
     */
    size_t execute_indexed_scan(
        const std::string& index_name,
        const std::string& column,
        const std::string& predicate,
        ExecutionContext& ctx);
    
    /**
     * @brief PHASE 4.3: Execute composite index scan for AND predicates
     * @param index_name Name of composite index
     * @param predicates Multiple predicates to apply
     * @param ctx Execution context
     * @return Number of matching rows
     */
    size_t execute_composite_indexed_scan(
        const std::string& index_name,
        const std::vector<std::string>& predicates,
        ExecutionContext& ctx);
    
    /**
     * @brief PHASE 4.3: Route filter execution to indexed or full scan
     * 
     * Makes decision based on Phase 4.2 optimizer recommendations:
     * - If optimization suggests index: attempt indexed scan
     * - If index unavailable: fallback to full scan
     * - If optimization suggests full scan: use full scan directly
     * 
     * @param condition WHERE clause condition
     * @param table_name Table being filtered
     * @param input_rows Total rows to filter
     * @param ctx Execution context
     * @return Number of filtered rows (0 = fallback to full scan)
     */
    size_t route_filter_execution(
        const std::string& condition,
        const std::string& table_name,
        size_t input_rows,
        ExecutionContext& ctx);
    
    /**
     * @brief PHASE 4.3: SIMD-optimized filter implementation
     * @param data Input batch data
     * @param predicate Filter condition
     * @return Output batch with filtered rows
     */
    std::vector<std::vector<uint8_t>> simd_filter(
        const std::vector<std::vector<uint8_t>>& data,
        const std::string& predicate);
    
    /**
     * @brief Vectorized sort implementation
     * @param data Batch to sort
     * @param sort_key Column to sort by
     * @return Sorted batch
     */
    std::vector<std::vector<uint8_t>> vectorized_sort(
        const std::vector<std::vector<uint8_t>>& data,
        const std::string& sort_key);
    
    /**
     * @brief Hash join implementation for two batches
     * @param left Left input batch
     * @param right Right input batch
     * @param join_key Key column for join
     * @return Joined result
     */
    std::vector<std::vector<uint8_t>> hash_join(
        const std::vector<std::vector<uint8_t>>& left,
        const std::vector<std::vector<uint8_t>>& right,
        const std::string& join_key);
};

} // namespace lyradb
