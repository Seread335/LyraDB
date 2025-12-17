#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <cstdint>
#include <limits>

namespace lyradb {
namespace optimization {

/**
 * @brief Index selection advisor for Phase 4.4
 * 
 * Analyzes query predicates and data statistics to intelligently choose
 * the best index strategy for query execution.
 * 
 * Features:
 * - Selectivity estimation
 * - Cost modeling for different index paths
 * - Adaptive index selection based on data distribution
 * - Pattern learning and caching of optimization decisions
 */
class IndexAdvisor {
public:
    /**
     * @brief Statistics about an index
     */
    struct IndexStats {
        std::string index_name;
        std::string column_name;
        size_t cardinality;      // Number of distinct values
        double avg_selectivity;  // Average query selectivity (0-1)
        size_t lookups_count;    // Number of times this index was used
        double avg_lookup_time_ms;
        bool is_composite;       // Is it a composite index?
    };
    
    /**
     * @brief Cost estimate for a query path
     */
    struct CostEstimate {
        std::string strategy;           // "full_scan", "index_btree", "composite_index"
        double estimated_cost;          // Cost units (lower is better)
        double estimated_rows;          // Expected rows to examine
        double estimated_speedup;       // Expected speedup vs full scan
        std::vector<std::string> indexes_used;
    };
    
    /**
     * @brief Index selection recommendation
     */
    struct SelectionRecommendation {
        std::string selected_strategy;  // Best strategy
        std::string primary_index;      // Primary index to use (if any)
        std::vector<std::string> secondary_indexes;
        double confidence;              // 0.0-1.0 confidence in recommendation
        CostEstimate full_scan_cost;
        CostEstimate indexed_scan_cost;
    };

    IndexAdvisor();
    ~IndexAdvisor();
    
    /**
     * @brief Recommend best index for a predicate
     * 
     * @param column_name Column being filtered
     * @param operator_type Comparison operator (=, <, >, <=, >=, !=)
     * @param table_size Total rows in table
     * @param available_indexes Available indexes on this column
     * @return Recommendation for which index/strategy to use
     */
    SelectionRecommendation recommend_index(
        const std::string& column_name,
        const std::string& operator_type,
        size_t table_size,
        const std::vector<IndexStats>& available_indexes);
    
    /**
     * @brief Estimate cost of different execution strategies
     * 
     * @param column_name Column being filtered
     * @param predicate_selectivity Expected fraction of matching rows (0-1)
     * @param table_size Total rows in table
     * @param index_stats Statistics about available indexes
     * @return Cost estimates for different strategies
     */
    std::vector<CostEstimate> estimate_costs(
        const std::string& column_name,
        double predicate_selectivity,
        size_t table_size,
        const std::vector<IndexStats>& index_stats);
    
    /**
     * @brief Learn from actual query execution results
     * 
     * Update statistics based on observed selectivity and performance
     * to improve future recommendations.
     */
    void learn_from_execution(
        const std::string& column_name,
        const std::string& strategy_used,
        size_t rows_examined,
        size_t rows_matched,
        double execution_time_ms);
    
    /**
     * @brief Check if predicate is selective (low cardinality)
     */
    static bool is_selective_predicate(
        double estimated_selectivity,
        size_t table_size);
    
    /**
     * @brief Estimate selectivity for a predicate
     * 
     * Uses column statistics to estimate what fraction of rows match
     * the given predicate.
     */
    double estimate_selectivity(
        const std::string& column_name,
        const std::string& operator_type,
        const std::string& value,
        const IndexStats& index_stats);
    
    /**
     * @brief Register index for future recommendations
     */
    void register_index(const IndexStats& stats);
    
    /**
     * @brief Get statistics for an index
     */
    const IndexStats* get_index_stats(const std::string& index_name) const;

private:
    // Index statistics cache
    std::unordered_map<std::string, IndexStats> index_cache_;
    
    // Query pattern learning
    struct QueryPattern {
        std::string column_name;
        std::string operator_type;
        int execution_count = 0;
        double avg_selectivity = 0.0;
        std::string best_strategy;
    };
    std::unordered_map<std::string, QueryPattern> pattern_cache_;
    
    /**
     * @brief Calculate cost model for full table scan
     */
    CostEstimate calculate_fullscan_cost(size_t table_size, double selectivity);
    
    /**
     * @brief Calculate cost model for B-tree index scan
     */
    CostEstimate calculate_btree_cost(
        const IndexStats& index,
        size_t table_size,
        double selectivity);
    
    /**
     * @brief Calculate cost model for composite index scan
     */
    CostEstimate calculate_composite_cost(
        const std::vector<IndexStats>& indexes,
        size_t table_size,
        double selectivity);
    
    /**
     * @brief Build query pattern key for caching
     */
    std::string build_pattern_key(
        const std::string& column,
        const std::string& op) const;
};

} // namespace optimization
} // namespace lyradb
