#pragma once

#include <string>
#include <vector>
#include <set>
#include <unordered_map>
#include <cstdint>

namespace lyradb {
namespace optimization {

/**
 * @brief Multi-column index optimizer for Phase 4.4
 * 
 * Handles optimization of queries with multiple predicates (AND/OR):
 * Example: WHERE age > 18 AND country = 'USA' AND salary < 100000
 * 
 * Strategies:
 * 1. Composite Index (best case): Single B-tree on (age, country, salary)
 * 2. Index Intersection (AND predicates): Result1 ∩ Result2 ∩ Result3
 * 3. Index Union (OR predicates): Result1 ∪ Result2 ∪ Result3
 * 4. Full Scan (fallback): Scan all rows
 */
class CompositeIndexOptimizer {
public:
    /**
     * @brief Predicate information
     */
    struct PredicateInfo {
        std::string column;
        std::string operator_type;  // =, <, >, <=, >=, !=, IN, BETWEEN
        std::string value;
        std::string logical_op;     // AND, OR (default AND)
        double estimated_selectivity;
    };
    
    /**
     * @brief Index intersection result
     */
    struct IntersectionResult {
        std::set<uint32_t> matching_rows;
        size_t estimated_cost;
        std::string cost_breakdown;  // For debugging
    };
    
    /**
     * @brief Index strategy choice
     */
    enum class Strategy {
        COMPOSITE_INDEX,      // Single multi-column index
        INDEX_INTERSECTION,   // Multiple single indexes AND
        INDEX_UNION,          // Multiple single indexes OR
        FULL_SCAN             // Fallback to scanning all rows
    };
    
    /**
     * @brief Optimization plan for multi-column query
     */
    struct OptimizationPlan {
        Strategy selected_strategy;
        std::vector<std::string> indexes_used;
        double estimated_speedup;
        std::string execution_order;  // For index intersection
        size_t estimated_cost;
    };

    CompositeIndexOptimizer();
    ~CompositeIndexOptimizer();
    
    /**
     * @brief Plan optimization for multiple predicates
     */
    OptimizationPlan plan_multi_predicate_query(
        const std::vector<PredicateInfo>& predicates,
        size_t table_size,
        const std::vector<std::string>& available_indexes);
    
    /**
     * @brief Execute index intersection for AND predicates
     * 
     * @param predicate_results Map of (column -> row IDs matching predicate)
     * @return Rows matching ALL predicates
     */
    IntersectionResult intersect_index_results(
        const std::unordered_map<std::string, std::set<uint32_t>>& predicate_results);
    
    /**
     * @brief Execute index union for OR predicates
     * 
     * @param predicate_results Map of (column -> row IDs matching predicate)
     * @return Rows matching ANY predicate
     */
    std::set<uint32_t> union_index_results(
        const std::unordered_map<std::string, std::set<uint32_t>>& predicate_results);
    
    /**
     * @brief Order predicates by selectivity for optimal intersection
     * 
     * Heuristic: Execute most selective predicates first to minimize
     * working set size during intersection.
     */
    std::vector<PredicateInfo> order_predicates_by_selectivity(
        const std::vector<PredicateInfo>& predicates);
    
    /**
     * @brief Check if a composite index exists for given columns
     */
    bool has_composite_index(
        const std::vector<std::string>& columns,
        const std::vector<std::string>& available_indexes) const;
    
    /**
     * @brief Estimate cost of index intersection
     * 
     * Cost model:
     * - Cost = sum of individual index scans + intersection cost
     * - Intersection done with sorted sets (O(k1 + k2))
     */
    size_t estimate_intersection_cost(
        const std::vector<PredicateInfo>& predicates,
        size_t table_size);
    
    /**
     * @brief Estimate cost of full scan with all predicates
     * 
     * Cost model: O(n) to scan all rows, evaluate all predicates
     */
    static size_t estimate_fullscan_cost(
        size_t table_size,
        size_t predicate_count);

private:
    /**
     * @brief Calculate selectivity product for AND predicates
     * 
     * For independent predicates: P(A AND B) = P(A) × P(B)
     */
    static double calculate_selectivity_and(
        const std::vector<PredicateInfo>& predicates);
    
    /**
     * @brief Calculate selectivity sum for OR predicates
     * 
     * For independent predicates: P(A OR B) = P(A) + P(B) - P(A) × P(B)
     */
    static double calculate_selectivity_or(
        const std::vector<PredicateInfo>& predicates);
    
    /**
     * @brief Determine optimal execution order for intersection
     * 
     * Greedy: Execute predicates in order of selectivity (most selective first)
     */
    std::vector<size_t> determine_intersection_order(
        const std::vector<PredicateInfo>& predicates);
};

} // namespace optimization
} // namespace lyradb
