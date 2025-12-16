#include "lyradb/composite_index_optimizer.h"
#include <algorithm>
#include <iterator>
#include <cmath>
#include <limits>

namespace lyradb {
namespace optimization {

CompositeIndexOptimizer::CompositeIndexOptimizer() {
}

CompositeIndexOptimizer::~CompositeIndexOptimizer() {
}

OptimizationPlan CompositeIndexOptimizer::plan_multi_predicate_query(
    const std::vector<PredicateInfo>& predicates,
    size_t table_size,
    const std::vector<std::string>& available_indexes) {
    
    OptimizationPlan plan;
    plan.estimated_cost = std::numeric_limits<size_t>::max();
    plan.selected_strategy = Strategy::FULL_SCAN;
    plan.estimated_speedup = 1.0;
    
    if (predicates.empty()) {
        plan.estimated_cost = table_size;
        return plan;
    }
    
    // Check if all predicates use AND
    bool all_and = true;
    for (const auto& pred : predicates) {
        if (pred.logical_op != "AND" && pred.logical_op != "") {
            all_and = false;
            break;
        }
    }
    
    // Strategy 1: Check for composite index
    std::vector<std::string> pred_columns;
    for (const auto& pred : predicates) {
        pred_columns.push_back(pred.column);
    }
    
    if (all_and && has_composite_index(pred_columns, available_indexes)) {
        size_t composite_cost = estimate_intersection_cost(predicates, table_size);
        double selectivity = calculate_selectivity_and(predicates);
        size_t matching_rows = static_cast<size_t>(table_size * selectivity);
        
        if (composite_cost < plan.estimated_cost) {
            plan.selected_strategy = Strategy::COMPOSITE_INDEX;
            plan.estimated_cost = composite_cost;
            plan.indexes_used.clear();
            plan.indexes_used.push_back("composite(" + predicates[0].column + ")");
            plan.estimated_speedup = static_cast<double>(table_size) / std::max(size_t(1), matching_rows + 10);
            plan.execution_order = "Single composite index scan";
        }
    }
    
    // Strategy 2: Index Intersection for AND predicates
    if (all_and) {
        size_t intersection_cost = estimate_intersection_cost(predicates, table_size);
        double selectivity = calculate_selectivity_and(predicates);
        size_t matching_rows = static_cast<size_t>(table_size * selectivity);
        
        if (intersection_cost < plan.estimated_cost) {
            plan.selected_strategy = Strategy::INDEX_INTERSECTION;
            plan.estimated_cost = intersection_cost;
            
            // Build execution order
            auto ordered = order_predicates_by_selectivity(predicates);
            plan.execution_order = "";
            plan.indexes_used.clear();
            for (size_t i = 0; i < ordered.size(); ++i) {
                plan.indexes_used.push_back(ordered[i].column);
                if (i > 0) plan.execution_order += " -> ";
                plan.execution_order += ordered[i].column;
            }
            
            plan.estimated_speedup = static_cast<double>(table_size) / std::max(size_t(1), matching_rows + 10);
        }
    }
    
    // Strategy 3: Full Scan (compare as fallback)
    size_t fullscan_cost = estimate_fullscan_cost(table_size, predicates.size());
    
    // Choose strategy
    if (fullscan_cost < plan.estimated_cost) {
        plan.selected_strategy = Strategy::FULL_SCAN;
        plan.estimated_cost = fullscan_cost;
        plan.indexes_used.clear();
        plan.estimated_speedup = 1.0;
        plan.execution_order = "Full table scan";
    }
    
    return plan;
}

IntersectionResult CompositeIndexOptimizer::intersect_index_results(
    const std::unordered_map<std::string, std::set<uint32_t>>& predicate_results) {
    
    IntersectionResult result;
    result.estimated_cost = 0;
    
    if (predicate_results.empty()) {
        result.cost_breakdown = "No predicates to intersect";
        return result;
    }
    
    // Initialize with first set
    auto it = predicate_results.begin();
    result.matching_rows = it->second;
    result.estimated_cost += it->second.size();
    result.cost_breakdown = "Start with " + std::to_string(it->second.size()) + " rows from " + it->first;
    
    ++it;
    
    // Intersect with remaining sets
    while (it != predicate_results.end()) {
        std::set<uint32_t> intersection;
        std::set_intersection(
            result.matching_rows.begin(), result.matching_rows.end(),
            it->second.begin(), it->second.end(),
            std::inserter(intersection, intersection.begin())
        );
        
        result.estimated_cost += result.matching_rows.size() + it->second.size();
        result.cost_breakdown += ", intersect with " + std::to_string(it->second.size()) + 
                                 " rows from " + it->first + " -> " + std::to_string(intersection.size());
        
        result.matching_rows = intersection;
        ++it;
    }
    
    return result;
}

std::set<uint32_t> CompositeIndexOptimizer::union_index_results(
    const std::unordered_map<std::string, std::set<uint32_t>>& predicate_results) {
    
    std::set<uint32_t> result;
    
    for (const auto& pair : predicate_results) {
        // Simple union: add all elements from this predicate result
        result.insert(pair.second.begin(), pair.second.end());
    }
    
    return result;
}

std::vector<CompositeIndexOptimizer::PredicateInfo> 
CompositeIndexOptimizer::order_predicates_by_selectivity(
    const std::vector<PredicateInfo>& predicates) {
    
    std::vector<PredicateInfo> ordered = predicates;
    
    // Sort by selectivity (ascending - most selective first)
    std::sort(ordered.begin(), ordered.end(),
        [](const PredicateInfo& a, const PredicateInfo& b) {
            return a.estimated_selectivity < b.estimated_selectivity;
        });
    
    return ordered;
}

bool CompositeIndexOptimizer::has_composite_index(
    const std::vector<std::string>& columns,
    const std::vector<std::string>& available_indexes) const {
    
    if (columns.empty()) return false;
    
    // Build potential composite index names
    std::string composite_name = "composite(";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) composite_name += ",";
        composite_name += columns[i];
    }
    composite_name += ")";
    
    // Check if it exists
    for (const auto& idx : available_indexes) {
        if (idx.find("composite") != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

size_t CompositeIndexOptimizer::estimate_intersection_cost(
    const std::vector<PredicateInfo>& predicates,
    size_t table_size) {
    
    // Cost = sum of (log n + k_i) for each index + intersection costs + overhead
    // where k_i is estimated matching rows for predicate i
    
    size_t total_cost = 0;
    
    for (const auto& pred : predicates) {
        // B-tree scan cost: O(log n + k)
        size_t log_cost = static_cast<size_t>(std::ceil(std::log2(table_size + 1))) * 2;
        size_t matching_rows = static_cast<size_t>(table_size * pred.estimated_selectivity);
        
        total_cost += log_cost + matching_rows;
    }
    
    // Phase 5 FIX: Add intersection overhead
    // Set intersection requires comparing and merging sorted sets
    // Overhead: ~200 cycles per intersection operation (setup + per-element operations)
    double intersection_selectivity = calculate_selectivity_and(predicates);
    size_t intersection_rows = static_cast<size_t>(table_size * intersection_selectivity);
    
    // Intersection cost model:
    // Setup overhead: 200 cycles per intersection
    // Merge cost: O(k1 + k2) to merge sorted sets
    // Total: OVERHEAD * num_predicates + (k1 + k2 + ... + kn)
    size_t per_intersection_overhead = 200;
    size_t total_overhead = per_intersection_overhead * (predicates.size() - 1);  // n-1 intersections
    
    // Merge work: sum of all intermediate result sizes
    size_t merge_work = 0;
    size_t cumulative_rows = static_cast<size_t>(table_size * predicates[0].estimated_selectivity);
    for (size_t i = 1; i < predicates.size(); ++i) {
        size_t next_rows = static_cast<size_t>(table_size * predicates[i].estimated_selectivity);
        // Cost to merge cumulative_rows and next_rows
        merge_work += cumulative_rows + next_rows;
        // Update cumulative (intersection)
        cumulative_rows = static_cast<size_t>(cumulative_rows * predicates[i].estimated_selectivity);
    }
    
    total_cost += total_overhead + merge_work;
    
    return total_cost;
}

size_t CompositeIndexOptimizer::estimate_fullscan_cost(
    size_t table_size,
    size_t predicate_count) {
    
    // Cost = n * predicate_count (evaluate all predicates on all rows)
    return table_size * predicate_count;
}

double CompositeIndexOptimizer::calculate_selectivity_and(
    const std::vector<PredicateInfo>& predicates) {
    
    double result = 1.0;
    
    for (const auto& pred : predicates) {
        result *= pred.estimated_selectivity;
    }
    
    return result;
}

double CompositeIndexOptimizer::calculate_selectivity_or(
    const std::vector<PredicateInfo>& predicates) {
    
    // P(A OR B) = P(A) + P(B) - P(A)*P(B)
    // Generalize: Use inclusion-exclusion
    
    double result = 0.0;
    
    for (const auto& pred : predicates) {
        result += pred.estimated_selectivity;
    }
    
    // Subtract pairwise products
    for (size_t i = 0; i < predicates.size(); ++i) {
        for (size_t j = i + 1; j < predicates.size(); ++j) {
            result -= predicates[i].estimated_selectivity * predicates[j].estimated_selectivity;
        }
    }
    
    return std::min(1.0, std::max(0.0, result));
}

std::vector<size_t> CompositeIndexOptimizer::determine_intersection_order(
    const std::vector<PredicateInfo>& predicates) {
    
    std::vector<size_t> order(predicates.size());
    for (size_t i = 0; i < predicates.size(); ++i) {
        order[i] = i;
    }
    
    // Sort by selectivity
    std::sort(order.begin(), order.end(),
        [&predicates](size_t a, size_t b) {
            return predicates[a].estimated_selectivity < predicates[b].estimated_selectivity;
        });
    
    return order;
}

} // namespace optimization
} // namespace lyradb
