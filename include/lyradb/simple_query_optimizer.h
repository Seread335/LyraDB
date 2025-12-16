#pragma once

#include <string>
#include <vector>
#include <map>
#include <set>

namespace lyradb {
namespace integration {

/**
 * @brief Simplified Phase 6 Query Optimizer for Real-World Integration
 * 
 * This module bridges between the query executor and optimization decision-making.
 * It provides a lightweight interface for optimizing WHERE clauses without
 * depending on complex Phase 4.4 modules.
 */
class SimpleQueryOptimizer {
public:
    /**
     * @brief Optimization strategy enum
     */
    enum class Strategy {
        FULL_SCAN,              // Fallback: scan all rows
        INDEX_SINGLE,           // Single index B-tree lookup
        INDEX_RANGE,            // Single index range scan
        INDEX_INTERSECTION,     // AND predicates with multiple indexes
        INDEX_UNION,            // OR predicates with multiple indexes
    };
    
    /**
     * @brief Optimization plan for a query
     */
    struct Plan {
        Strategy strategy;
        double predicted_speedup;
        std::vector<std::string> indexes_to_use;
        std::string explanation;
    };
    
    SimpleQueryOptimizer();
    ~SimpleQueryOptimizer();
    
    /**
     * @brief Optimize a WHERE clause
     * 
     * @param where_clause The WHERE clause text (e.g., "age = 25")
     * @param table_size Number of rows in the table
     * @param available_indexes List of available index names
     * @return Optimization plan with strategy and estimated speedup
     */
    Plan optimize(
        const std::string& where_clause,
        size_t table_size,
        const std::vector<std::string>& available_indexes);
    
    /**
     * @brief Register an index for this table
     */
    void register_index(
        const std::string& index_name,
        const std::string& column_name);
    
    /**
     * @brief Record actual execution result for learning
     */
    void record_result(
        const std::string& where_clause,
        Strategy strategy_used,
        size_t rows_examined,
        size_t rows_matched,
        double execution_time_ms);

private:
    struct IndexInfo {
        std::string name;
        std::string column;
    };
    
    std::map<std::string, IndexInfo> indexes_;  // column -> index info
    size_t total_queries_optimized_;
    double total_predicted_speedup_;
    
    // Helper methods
    bool parse_equality_predicate(
        const std::string& where_clause,
        std::string& column,
        std::string& value);
    
    bool parse_range_predicate(
        const std::string& where_clause,
        std::string& column,
        std::string& op,
        std::string& value);
    
    size_t count_predicates(const std::string& where_clause);
};

} // namespace integration
} // namespace lyradb
