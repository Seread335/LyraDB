#include "lyradb/index_advisor.h"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace lyradb {
namespace optimization {

IndexAdvisor::IndexAdvisor() = default;
IndexAdvisor::~IndexAdvisor() = default;

SelectionRecommendation IndexAdvisor::recommend_index(
    const std::string& column_name,
    const std::string& operator_type,
    size_t table_size,
    const std::vector<IndexStats>& available_indexes) {
    
    SelectionRecommendation recommendation;
    recommendation.confidence = 0.0;
    
    if (table_size == 0) {
        recommendation.selected_strategy = "full_scan";
        recommendation.primary_index = "";
        return recommendation;
    }
    
    // Get pattern history if available
    std::string pattern_key = build_pattern_key(column_name, operator_type);
    auto pattern_it = pattern_cache_.find(pattern_key);
    
    // Estimate selectivity based on column and operator
    double estimated_selectivity = 0.5;  // Default: 50% selectivity
    
    if (pattern_it != pattern_cache_.end()) {
        // Use learned selectivity from previous executions
        estimated_selectivity = pattern_it->second.avg_selectivity;
        std::cout << "[Phase 4.4 ADVISOR] Using learned selectivity for " << pattern_key 
                  << ": " << (estimated_selectivity * 100) << "%\n";
    } else {
        // Estimate based on operator type
        if (operator_type == "=" || operator_type == "==") {
            // Equality is usually selective
            if (!available_indexes.empty()) {
                estimated_selectivity = 1.0 / available_indexes[0].cardinality;
            }
        } else if (operator_type == "<" || operator_type == ">" || 
                   operator_type == "<=" || operator_type == ">=") {
            // Range queries typically select 20-50% of data
            estimated_selectivity = 0.25;
        } else if (operator_type == "!=" || operator_type == "<>") {
            // Not-equal typically selects most rows
            estimated_selectivity = 0.8;
        }
    }
    
    // Cost estimates for different strategies
    auto costs = estimate_costs(column_name, estimated_selectivity, table_size, available_indexes);
    
    // Find best cost strategy
    double best_cost = std::numeric_limits<double>::max();
    std::string best_strategy = "full_scan";
    std::string best_index = "";
    
    for (const auto& cost : costs) {
        std::cout << "[Phase 4.4] Strategy " << cost.strategy << ": cost=" << cost.estimated_cost 
                  << ", speedup=" << cost.estimated_speedup << "x\n";
        
        if (cost.estimated_cost < best_cost) {
            best_cost = cost.estimated_cost;
            best_strategy = cost.strategy;
            if (!cost.indexes_used.empty()) {
                best_index = cost.indexes_used[0];
            }
        }
    }
    
    // Build recommendation
    recommendation.selected_strategy = best_strategy;
    recommendation.primary_index = best_index;
    
    // Calculate confidence based on cost difference
    if (costs.size() >= 2) {
        double fullscan_cost = 0, indexed_cost = std::numeric_limits<double>::max();
        
        for (const auto& cost : costs) {
            if (cost.strategy == "full_scan") {
                fullscan_cost = cost.estimated_cost;
                recommendation.full_scan_cost = cost;
            } else if (cost.strategy.find("index") != std::string::npos) {
                if (cost.estimated_cost < indexed_cost) {
                    indexed_cost = cost.estimated_cost;
                    recommendation.indexed_scan_cost = cost;
                }
            }
        }
        
        // Confidence: higher when there's clear cost difference
        if (fullscan_cost > 0 && indexed_cost < fullscan_cost) {
            recommendation.confidence = std::min(1.0, 
                (fullscan_cost - indexed_cost) / fullscan_cost);
        }
    }
    
    std::cout << "[Phase 4.4 ADVISOR] Selected: " << best_strategy 
              << " (confidence: " << (recommendation.confidence * 100) << "%)\n";
    
    return recommendation;
}

std::vector<CostEstimate> IndexAdvisor::estimate_costs(
    const std::string& column_name,
    double predicate_selectivity,
    size_t table_size,
    const std::vector<IndexStats>& index_stats) {
    
    std::vector<CostEstimate> costs;
    
    // Strategy 1: Full table scan
    costs.push_back(calculate_fullscan_cost(table_size, predicate_selectivity));
    
    // Strategy 2: B-tree index scan (if indexes available)
    for (const auto& index : index_stats) {
        if (!index.is_composite) {
            costs.push_back(calculate_btree_cost(index, table_size, predicate_selectivity));
        }
    }
    
    // Strategy 3: Composite index (if available)
    std::vector<IndexStats> composite_indexes;
    for (const auto& index : index_stats) {
        if (index.is_composite) {
            composite_indexes.push_back(index);
        }
    }
    if (!composite_indexes.empty()) {
        costs.push_back(calculate_composite_cost(composite_indexes, table_size, predicate_selectivity));
    }
    
    return costs;
}

void IndexAdvisor::learn_from_execution(
    const std::string& column_name,
    const std::string& strategy_used,
    size_t rows_examined,
    size_t rows_matched,
    double execution_time_ms) {
    
    if (rows_examined == 0) return;
    
    double actual_selectivity = static_cast<double>(rows_matched) / rows_examined;
    
    std::cout << "[Phase 4.4 LEARNING] Column: " << column_name 
              << " | Strategy: " << strategy_used
              << " | Selectivity: " << (actual_selectivity * 100) << "%"
              << " | Time: " << execution_time_ms << "ms\n";
    
    // Update pattern cache (simplified - would need operator type in real implementation)
    std::string pattern_key = build_pattern_key(column_name, "=");
    auto& pattern = pattern_cache_[pattern_key];
    pattern.column_name = column_name;
    pattern.execution_count++;
    
    // Update moving average of selectivity
    if (pattern.execution_count == 1) {
        pattern.avg_selectivity = actual_selectivity;
    } else {
        double alpha = 0.3;  // Learning rate
        pattern.avg_selectivity = (1 - alpha) * pattern.avg_selectivity + alpha * actual_selectivity;
    }
    
    pattern.best_strategy = strategy_used;
}

bool IndexAdvisor::is_selective_predicate(
    double estimated_selectivity,
    size_t table_size) {
    
    // A predicate is selective if it filters out most rows
    // Rule: selectivity < 10% OR fewer than 1000 matching rows
    return estimated_selectivity < 0.1 || (table_size * estimated_selectivity) < 1000;
}

double IndexAdvisor::estimate_selectivity(
    const std::string& column_name,
    const std::string& operator_type,
    const std::string& value,
    const IndexStats& index_stats) {
    
    // Rough selectivity estimation based on operator and cardinality
    double selectivity = 0.5;
    
    if (operator_type == "=" || operator_type == "==") {
        // Assuming uniform distribution: 1 / cardinality
        selectivity = 1.0 / std::max(1.0, static_cast<double>(index_stats.cardinality));
    } else if (operator_type == "<" || operator_type == ">") {
        // Range: assume half the data
        selectivity = 0.5;
    } else if (operator_type == "<=" || operator_type == ">=") {
        // Inclusive range: slightly more than exclusive
        selectivity = 0.55;
    } else if (operator_type == "!=" || operator_type == "<>") {
        // Not equal: all but one value
        selectivity = 1.0 - (1.0 / std::max(1.0, static_cast<double>(index_stats.cardinality)));
    }
    
    return selectivity;
}

void IndexAdvisor::register_index(const IndexStats& stats) {
    index_cache_[stats.index_name] = stats;
    std::cout << "[Phase 4.4] Registered index: " << stats.index_name 
              << " on column: " << stats.column_name 
              << " (cardinality: " << stats.cardinality << ")\n";
}

const IndexAdvisor::IndexStats* IndexAdvisor::get_index_stats(
    const std::string& index_name) const {
    
    auto it = index_cache_.find(index_name);
    if (it != index_cache_.end()) {
        return &it->second;
    }
    return nullptr;
}

IndexAdvisor::CostEstimate IndexAdvisor::calculate_fullscan_cost(
    size_t table_size, 
    double selectivity) {
    
    CostEstimate cost;
    cost.strategy = "full_scan";
    cost.estimated_rows = table_size;
    
    // Full scan: O(n) where n = table_size
    // Cost = table_size (scan all rows)
    // Phase 5 FIX: Account for predicate evaluation cost
    // ~2 cycles per row (comparison + branch)
    cost.estimated_cost = table_size * 2;
    cost.estimated_speedup = 1.0;  // Baseline
    
    return cost;
}

IndexAdvisor::CostEstimate IndexAdvisor::calculate_btree_cost(
    const IndexStats& index,
    size_t table_size,
    double selectivity) {
    
    CostEstimate cost;
    cost.strategy = "index_btree";
    cost.indexes_used.push_back(index.index_name);
    
    // Phase 5 FIX: Add selectivity threshold check
    // If > 50% of rows match, full scan is better (no index overhead)
    if (selectivity > 0.5) {
        // Fall back to full scan for low-selectivity predicates
        cost = calculate_fullscan_cost(table_size, selectivity);
        return cost;
    }
    
    // B-tree: O(log n + k) where:
    // - log n = tree traversal cost
    // - k = rows examined in result set
    size_t matching_rows = std::max(1UL, static_cast<size_t>(table_size * selectivity));
    size_t log_cost = static_cast<size_t>(std::ceil(std::log2(table_size)) * 2);  // ~2x per node
    
    // Phase 5 FIX: Add index overhead (cache misses, random I/O)
    // Index lookup has ~100-200 cycle overhead per search
    size_t index_overhead = 150;
    
    cost.estimated_cost = index_overhead + log_cost + matching_rows;
    cost.estimated_rows = matching_rows;
    cost.estimated_speedup = static_cast<double>(table_size * 2) / std::max(1UL, cost.estimated_cost);
    
    return cost;
}

IndexAdvisor::CostEstimate IndexAdvisor::calculate_composite_cost(
    const std::vector<IndexStats>& indexes,
    size_t table_size,
    double selectivity) {
    
    CostEstimate cost;
    cost.strategy = "composite_index";
    
    for (const auto& index : indexes) {
        cost.indexes_used.push_back(index.index_name);
    }
    
    // Composite index: O(log n + k) similar to single index
    // But with better selectivity due to multiple predicates
    size_t matching_rows = std::max(1UL, static_cast<size_t>(table_size * selectivity * selectivity));
    size_t log_cost = static_cast<size_t>(std::ceil(std::log2(table_size)) * 2.5);  // Slightly higher
    
    cost.estimated_cost = log_cost + matching_rows;
    cost.estimated_rows = matching_rows;
    cost.estimated_speedup = static_cast<double>(table_size) / cost.estimated_cost;
    
    return cost;
}

std::string IndexAdvisor::build_pattern_key(
    const std::string& column,
    const std::string& op) const {
    
    return column + ":" + op;
}

} // namespace optimization
} // namespace lyradb
