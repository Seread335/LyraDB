#pragma once

#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

namespace lyradb {

// Forward declarations
class Database;
class QueryExecutor;
class ExpressionEvaluator;
namespace query {
    class SqlParser;
    class SelectStatement;
}
namespace plan {
    class QueryOptimizer;
    class QueryPlan;
}

/**
 * @brief Query Execution Pipeline Integration Layer
 * 
 * High-level orchestration of complete SQL query execution pipeline:
 * 
 * Architecture:
 * ┌─────────────────────────────────────────────────────────────┐
 * │  SQL Query String                                           │
 * └────────────────────────┬────────────────────────────────────┘
 *                          │ parse()
 * ┌────────────────────────▼────────────────────────────────────┐
 * │  1. Parser (SqlParser)                                      │
 * │     - Lexical analysis (tokenization)                       │
 * │     - Syntax analysis (recursive descent)                   │
 * │     - Result: Abstract Syntax Tree (SelectStatement)        │
 * └────────────────────────┬────────────────────────────────────┘
 *                          │ validate_query()
 * ┌────────────────────────▼────────────────────────────────────┐
 * │  2. Validation Layer                                        │
 * │     - Schema validation (table exists)                      │
 * │     - Column reference validation                           │
 * │     - Type compatibility checking                           │
 * └────────────────────────┬────────────────────────────────────┘
 *                          │ optimize()
 * ┌────────────────────────▼────────────────────────────────────┐
 * │  3. Query Optimizer (QueryOptimizer)                        │
 * │     - Predicate pushdown                                    │
 * │     - Column pruning                                        │
 * │     - Join reordering                                       │
 * │     - Sort elimination                                      │
 * │     - Result: Optimized QueryPlan                           │
 * └────────────────────────┬────────────────────────────────────┘
 *                          │ execute()
 * ┌────────────────────────▼────────────────────────────────────┐
 * │  4. Query Executor (QueryExecutor)                          │
 * │     - Vectorized batch processing (1024 rows default)       │
 * │     - Operator execution: TableScan, Filter, Project        │
 * │                          Join, Aggregate, Sort, Limit       │
 * │     - SIMD-optimized filters and sorts                      │
 * │     - Result: ExecutionContext with batch data              │
 * └────────────────────────┬────────────────────────────────────┘
 *                          │ evaluate_expressions()
 * ┌────────────────────────▼────────────────────────────────────┐
 * │  5. Expression Evaluator (ExpressionEvaluator)              │
 * │     - Recursive expression tree evaluation                  │
 * │     - Type coercion (int64, double, string, bool)           │
 * │     - Arithmetic, comparison, logical operations            │
 * │     - String and math functions (20+ operators)             │
 * │     - NULL propagation semantics                            │
 * │     - Result: Evaluated values for computed columns         │
 * └────────────────────────┬────────────────────────────────────┘
 *                          │ materialize_results()
 * ┌────────────────────────▼────────────────────────────────────┐
 * │  QueryResult                                                │
 * │  - Column names and types                                   │
 * │  - Result rows (2D matrix)                                  │
 * │  - Execution statistics (rows processed, time)              │
 * │  - Output formatters: CSV, JSON, ASCII Table                │
 * └─────────────────────────────────────────────────────────────┘
 * 
 * Performance Characteristics:
 * - Single-threaded synchronous execution
 * - Vectorized operators process 1K-8K rows per batch
 * - SIMD-accelerated filters and sorts (optional)
 * - Zero-copy batch processing where possible
 * - Typical latency: < 100ms for 10K row queries
 * 
 * Error Handling:
 * - Comprehensive validation at each pipeline stage
 * - Descriptive error messages with context
 * - Exception-based error reporting
 * 
 * Thread Safety:
 * - Not thread-safe; requires external synchronization
 * - Each thread should use its own QueryExecutionEngine instance
 */
class QueryExecutionEngine {
public:
    /**
     * @brief Create query execution engine for a database
     * @param database Reference to database for schema and table access
     * @throws std::runtime_error if database pointer is null
     */
    explicit QueryExecutionEngine(Database* database);
    
    ~QueryExecutionEngine();
    
    /**
     * @brief Execute a SQL query end-to-end
     * 
     * Orchestrates complete pipeline:
     * 1. Parse SQL → SelectStatement
     * 2. Validate schema and references
     * 3. Optimize → QueryPlan
     * 4. Execute → ExecutionContext
     * 5. Materialize → QueryResult
     * 
     * @param sql SQL query string
     *     Examples:
     *     - "SELECT * FROM users"
     *     - "SELECT id, name FROM users WHERE age > 18 ORDER BY name LIMIT 10"
     *     - "SELECT dept, COUNT(*) FROM employees GROUP BY dept"
     * 
     * @return Query execution result with rows and metadata
     * @throws std::runtime_error on:
     *     - Empty query string
     *     - Parse errors (invalid syntax)
     *     - Schema validation errors (table not found)
     *     - Type errors
     *     - Execution errors
     */
    struct QueryResult {
        std::vector<std::string> column_names;
        std::vector<std::vector<std::string>> rows;
        uint64_t rows_processed = 0;
        uint64_t rows_returned = 0;
        std::string execution_plan;
        double execution_time_ms = 0.0;
        
        /**
         * @brief Get result as CSV string
         */
        std::string to_csv() const;
        
        /**
         * @brief Get result as JSON string
         */
        std::string to_json() const;
        
        /**
         * @brief Pretty print result as table
         */
        std::string to_table() const;
    };
    
    QueryResult execute(const std::string& sql);
    
    /**
     * @brief Get last query execution plan
     */
    std::string get_last_execution_plan() const;
    
    /**
     * @brief Set batch size for query execution
     * @param size Number of rows per batch (default: 1024)
     */
    void set_batch_size(size_t size);
    
    /**
     * @brief Enable/disable SIMD optimizations
     */
    void set_simd_enabled(bool enable);
    
    /**
     * @brief Get execution statistics
     */
    struct ExecutionStats {
        uint64_t total_rows_processed = 0;
        uint64_t total_queries_executed = 0;
        double total_execution_time_ms = 0.0;
        std::string last_error;
    };
    
    const ExecutionStats& get_stats() const;

private:
    Database* database_;
    std::unique_ptr<query::SqlParser> parser_;
    std::unique_ptr<plan::QueryOptimizer> optimizer_;
    std::unique_ptr<QueryExecutor> executor_;
    std::unique_ptr<ExpressionEvaluator> evaluator_;
    
    ExecutionStats stats_;
    std::string last_execution_plan_;
    size_t batch_size_;
    bool simd_enabled_;
    
    // Helper methods
    void validate_query(const query::SelectStatement& stmt);
    void validate_schema(const std::string& table_name);
};

} // namespace lyradb
