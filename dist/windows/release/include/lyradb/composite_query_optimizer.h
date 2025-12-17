/**
 * @file composite_query_optimizer.h
 * @brief Composite query optimizer combining range predicates with index selection
 * 
 * Phase 4.2: B-Tree Query Optimization
 * Integrates RangeQueryOptimizer with IndexAwareOptimizer for automatic
 * index selection and query plan generation
 * 
 * Author: LyraDB Team
 * Date: 2024
 */

#ifndef LYRADB_COMPOSITE_QUERY_OPTIMIZER_H
#define LYRADB_COMPOSITE_QUERY_OPTIMIZER_H

#include "range_query_optimizer.h"
#include "query_plan.h"
#include <memory>
#include <vector>
#include <map>
#include <string>

namespace lyradb {

// Use RangeQueryOptimizer from query namespace
using query::RangeQueryOptimizer;
using query::RangePredicate;

/**
 * @class CompositeQueryOptimizer
 * @brief Combines range predicate detection with B-tree index selection
 * 
 * This optimizer:
 * 1. Detects range predicates in WHERE clauses using RangeQueryOptimizer
 * 2. Recommends appropriate B-tree indexes based on predicates
 * 3. Estimates selectivity and execution cost
 * 4. Generates optimized query plans using B-tree indexes when beneficial
 * 5. Falls back to full table scan when index overhead exceeds benefit
 */
class CompositeQueryOptimizer {
public:
    /**
     * @struct OptimizationDecision
     * @brief Decision result for index usage
     */
    struct OptimizationDecision {
        bool use_index = false;                 ///< Whether to use B-tree index
        bool use_multiple_indexes = false;      ///< Whether to use composite index
        std::string primary_index = "";         ///< Primary index to use
        std::vector<std::string> indexes = {};  ///< All recommended indexes
        double estimated_selectivity = 1.0;     ///< Estimated selectivity
        double estimated_speedup = 1.0;         ///< Estimated speedup factor
        std::string reason = "";                ///< Why this decision was made
        
        /**
         * Get human-readable description
         */
        std::string to_string() const;
    };
    
    /**
     * @struct OptimizationStats
     * @brief Statistics about optimization analysis
     */
    struct OptimizationStats {
        size_t queries_analyzed = 0;
        size_t queries_optimized = 0;
        size_t range_predicates_found = 0;
        size_t indexes_recommended = 0;
        double total_estimated_speedup = 0.0;
        double avg_selectivity = 0.0;
        
        /**
         * Get human-readable statistics
         */
        std::string to_string() const;
    };

    /**
     * Default constructor
     */
    CompositeQueryOptimizer();
    
    /**
     * Destructor
     */
    ~CompositeQueryOptimizer();
    
    /**
     * Analyze a query and determine optimal index usage
     * 
     * @param table_name Name of table being queried
     * @param where_clause WHERE clause string
     * @param table_size Number of rows in table
     * @param available_indexes Available indexes on table
     * 
     * @return OptimizationDecision with recommendation
     */
    OptimizationDecision analyze_query(
        const std::string& table_name,
        const std::string& where_clause,
        size_t table_size,
        const std::vector<std::string>& available_indexes = {}
    );
    
    /**
     * Optimize a query plan based on detected range predicates
     * 
     * @param plan Input query plan
     * @param table_size Size of main table
     * @param available_indexes Available indexes
     * 
     * @return Optimized query plan with B-tree indexes if beneficial
     */
    std::shared_ptr<plan::PlanNode> optimize_plan(
        std::shared_ptr<plan::PlanNode> plan,
        size_t table_size,
        const std::vector<std::string>& available_indexes = {}
    );
    
    /**
     * Determine if B-tree index should be created for a column
     * 
     * @param column_name Column to potentially index
     * @param table_size Table size
     * @param estimated_selectivity Estimated selectivity of range queries
     * 
     * @return true if index creation is recommended
     */
    bool recommend_index_creation(
        const std::string& column_name,
        size_t table_size,
        double estimated_selectivity
    );
    
    /**
     * Get optimization statistics
     * 
     * @return Statistics about queries analyzed
     */
    const OptimizationStats& get_stats() const;
    
    /**
     * Reset optimization statistics
     */
    void reset_stats();
    
    /**
     * Set verbosity for optimization reporting
     * 
     * @param verbose true to enable detailed output
     */
    void set_verbose(bool verbose) { verbose_ = verbose; }
    
    /**
     * Get current verbosity setting
     */
    bool is_verbose() const { return verbose_; }
    
    /**
     * Calculate cost of full table scan
     * 
     * @param table_size Number of rows to scan
     * 
     * @return Relative cost (higher = slower)
     */
    double calculate_scan_cost(size_t table_size) const;
    
    /**
     * Calculate cost of B-tree indexed scan
     * 
     * @param table_size Total rows in table
     * @param selectivity Estimated selectivity
     * 
     * @return Relative cost (higher = slower)
     */
    double calculate_index_cost(size_t table_size, double selectivity) const;
    
    /**
     * Choose optimal index from available options
     * 
     * @param available_indexes List of available indexes
     * @param selectivity Estimated selectivity
     * @param table_size Table size
     * 
     * @return Best index to use, or empty string for full scan
     */
    std::string choose_best_index(
        const std::vector<std::string>& available_indexes,
        double selectivity,
        size_t table_size
    ) const;

private:
    /**
     * Estimate selectivity based on predicate type and value range
     * 
     * @param where_clause WHERE clause to analyze
     * @param table_size Total rows in table
     * 
     * @return Estimated selectivity (0.0 to 1.0)
     */
    double estimate_selectivity_from_clause(
        const std::string& where_clause,
        size_t table_size
    );
    
    /**
     * Decide whether to use B-tree index based on selectivity and table size
     * 
     * @param selectivity Estimated selectivity
     * @param table_size Total rows
     * 
     * @return true if B-tree index should be used
     */
    bool should_use_index(double selectivity, size_t table_size);
    
    /**
     * Calculate estimated speedup from using B-tree
     * 
     * @param selectivity Estimated selectivity
     * @param table_size Total rows
     * 
     * @return Speedup factor (e.g., 2.0 = 2x faster)
     */
    double estimate_speedup(double selectivity, size_t table_size);

    // Configuration
    bool verbose_ = false;
    
    // Thresholds for optimization decisions
    static constexpr double SELECTIVITY_THRESHOLD = 0.5;  // Use index if <50% selectivity
    static constexpr size_t MIN_TABLE_SIZE = 1000;        // Don't index tables <1K rows
    static constexpr double MIN_SPEEDUP = 1.3;            // Need at least 1.3x speedup
    
    // Statistics tracking
    OptimizationStats stats_;
    
    // Range query optimizer instance (forward declared, pimpl pattern)
    std::unique_ptr<RangeQueryOptimizer> range_optimizer_;
};

} // namespace lyradb

#endif // LYRADB_COMPOSITE_QUERY_OPTIMIZER_H
