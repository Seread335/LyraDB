#pragma once

#include "lyradb/index_advisor.h"
#include "lyradb/composite_index_optimizer.h"
#include <string>
#include <vector>
#include <memory>

namespace lyradb {
namespace integration {

/**
 * @brief Phase 4.4 Query Optimization Integration
 * 
 * Orchestrates the use of IndexAdvisor, CompositeIndexOptimizer, and QueryRewriter
 * to optimize query execution. This module bridges the optimization modules with
 * the actual query executor.
 */
class Phase44QueryOptimizer {
public:
    /**
     * @brief Optimization decision for a query
     */
    struct QueryOptimizationPlan {
        std::string query_text;
        std::string optimized_query;
        std::string strategy;  // full_scan, index_single, index_composite, index_intersection
        std::vector<std::string> indexes_used;
        double predicted_speedup;
        std::string execution_notes;
    };
    
    Phase44QueryOptimizer();
    ~Phase44QueryOptimizer();
    
    /**
     * @brief Analyze and optimize a WHERE clause
     * 
     * @param where_clause The original WHERE clause
     * @param table_size Estimated rows in table
     * @param available_indexes List of available index names
     * @return Optimization plan with strategy and predicted speedup
     */
    QueryOptimizationPlan optimize_where_clause(
        const std::string& where_clause,
        size_t table_size,
        const std::vector<std::string>& available_indexes);
    
    /**
     * @brief Register an index for optimization decisions
     */
    void register_index(
        const std::string& index_name,
        const std::string& column_name,
        size_t cardinality,
        bool is_composite = false);
    
    /**
     * @brief Update cost model based on actual execution
     */
    void record_execution_result(
        const std::string& query,
        const std::string& strategy_used,
        size_t rows_examined,
        size_t rows_matched,
        double execution_time_ms);
    
    /**
     * @brief Get current optimization statistics
     */
    std::string get_optimization_stats() const;

private:
    optimization::IndexAdvisor index_advisor_;
    optimization::CompositeIndexOptimizer composite_optimizer_;
    
    size_t total_queries_optimized_;
    double total_speedup_;
    
    /**
     * @brief Parse WHERE clause into structured format
     */
    struct ParsedPredicate {
        std::string column;
        std::string op;
        std::string value;
    };
    
    std::vector<ParsedPredicate> parse_where_clause(const std::string& where_clause);
    
    /**
     * @brief Check if predicate is single column or composite
     */
    bool is_composite_predicate(const std::vector<ParsedPredicate>& predicates);
    
    /**
     * @brief Estimate selectivity for a predicate
     */
    double estimate_selectivity(const ParsedPredicate& pred);
};

} // namespace integration
} // namespace lyradb
