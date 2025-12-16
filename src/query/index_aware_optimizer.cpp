#include "lyradb/index_aware_optimizer.h"
#include "lyradb/query_plan.h"
#include "lyradb/index_manager.h"
#include <algorithm>
#include <regex>
#include <cmath>
#include <cctype>
#include <iostream>

namespace lyradb {
namespace plan {

// ============================================================================
// IndexedScanNode Implementation
// ============================================================================

std::string IndexedScanNode::to_string() const {
    std::string result = "IndexedScan[table=" + table_name_;
    if (uses_index_) {
        result += ", index=" + index_name_ + " on " + index_column_;
        result += " (" + index_type_ + ")";
        result += ", est_rows=" + std::to_string(estimated_rows_);
    }
    result += "]";
    return result;
}

long long IndexedScanNode::estimated_memory() const {
    // Assume ~100 bytes per row average
    return estimated_rows_ * 100;
}

// ============================================================================
// IndexedFilterNode Implementation
// ============================================================================

std::string IndexedFilterNode::to_string() const {
    std::string result = "Filter[";
    result += "predicate=" + predicate_column_;
    result += ", selectivity=" + std::to_string(selectivity_);
    result += "]";
    return result;
}

long long IndexedFilterNode::estimated_rows() const {
    if (!child_) return 0;
    long long child_rows = child_->estimated_rows();
    return static_cast<long long>(child_rows * selectivity_);
}

long long IndexedFilterNode::estimated_memory() const {
    if (!child_) return 0;
    return child_->estimated_memory();
}

std::vector<PlanNode*> IndexedFilterNode::children() {
    return {child_.get()};
}

const std::vector<PlanNode*> IndexedFilterNode::children() const {
    return {child_.get()};
}

// ============================================================================
// IndexAwareOptimizer Implementation
// ============================================================================

std::unique_ptr<QueryPlan> IndexAwareOptimizer::optimize_with_indexes(const QueryPlan& plan) {
    if (!index_manager_) {
        return std::make_unique<QueryPlan>(
            std::make_unique<ScanNode>("", 0, 0)
        );
    }

    // Analyze opportunities
    auto opportunities = analyze_index_opportunities(plan);

    // Start with the original plan
    auto optimized_plan = std::make_unique<QueryPlan>(
        std::make_unique<ScanNode>(
            dynamic_cast<const ScanNode*>(plan.root())->table_name(),
            dynamic_cast<const ScanNode*>(plan.root())->row_count(),
            dynamic_cast<const ScanNode*>(plan.root())->column_count()
        )
    );

    // Apply best index selection if one was found
    if (!opportunities.empty()) {
        optimized_plan = apply_index_selection(plan, opportunities[0]);
    }

    return optimized_plan;
}

std::vector<IndexSelectionStats> IndexAwareOptimizer::analyze_index_opportunities(const QueryPlan& plan) {
    std::vector<IndexSelectionStats> opportunities;

    // For now, return empty - this requires const_cast which should be redesigned
    // to properly handle const traversal in future versions
    
    return opportunities;
}

std::unique_ptr<QueryPlan> IndexAwareOptimizer::apply_index_selection(
    const QueryPlan& plan,
    const IndexSelectionStats& stats) {
    
    // Create an indexed scan node
    auto indexed_scan = std::make_unique<IndexedScanNode>(
        stats.table_name,
        stats.row_count,
        1  // column_count placeholder
    );

    // Simplified: just mark as indexed without accessing IndexManager
    indexed_scan->use_index("idx_auto", stats.column_name, "BTree");
    
    // Estimate rows after index
    long long estimated_rows = static_cast<long long>(stats.row_count * stats.selectivity);
    indexed_scan->set_estimated_rows(estimated_rows);

    return std::make_unique<QueryPlan>(std::move(indexed_scan));
}

IndexedFilterNode::PredicateType IndexAwareOptimizer::analyze_predicate(
    const std::string& condition,
    std::string& column_name) {
    
    // Clean up the condition string
    std::string clean_cond = condition;
    
    // Convert to uppercase for case-insensitive matching
    std::string upper_cond;
    for (char c : condition) {
        upper_cond += std::toupper(static_cast<unsigned char>(c));
    }
    
    column_name.clear();
    
    // Extract column name - word characters before operator
    std::regex column_regex(R"(^[\s]*(\w+)\s*(?:=|<|>|!=|<=|>=|IN|BETWEEN))");
    std::smatch match;
    
    if (std::regex_search(condition, match, column_regex)) {
        column_name = match[1].str();
    }

    // Determine predicate type based on operators
    if (upper_cond.find("IN") != std::string::npos &&
        upper_cond.find("BETWEEN") == std::string::npos) {
        // Make sure it's not BETWEEN
        return IndexedFilterNode::PredicateType::IN_LIST;
    } 
    else if (upper_cond.find("BETWEEN") != std::string::npos) {
        return IndexedFilterNode::PredicateType::BETWEEN;
    } 
    else if (condition.find("!=") != std::string::npos || 
             condition.find("<>") != std::string::npos) {
        return IndexedFilterNode::PredicateType::NOT_EQUAL;
    } 
    else if ((condition.find(">") != std::string::npos ||
              condition.find("<") != std::string::npos) &&
             condition.find("IN") == std::string::npos) {
        // Range predicate: >, <, >=, <=
        return IndexedFilterNode::PredicateType::RANGE;
    } 
    else if (condition.find("=") != std::string::npos &&
             condition.find("!=") == std::string::npos &&
             condition.find("<=") == std::string::npos &&
             condition.find(">=") == std::string::npos) {
        return IndexedFilterNode::PredicateType::EQUALITY;
    }

    // Default to equality if uncertain
    return IndexedFilterNode::PredicateType::EQUALITY;
}

double IndexAwareOptimizer::estimate_scan_cost(const IndexSelectionStats& stats) {
    // Full table scan cost calculation
    // Includes reading rows + filtering + evaluating predicates
    
    // Base: 1M rows = 1.0 cost unit
    double scan_cost = static_cast<double>(stats.row_count) / 1000000.0;
    
    // Filtering cost: proportional to how selective the predicate is
    // Low selectivity = need to check many rows
    double filter_cost = (1.0 - stats.selectivity) * 0.5;
    
    return FULL_SCAN_COST * (scan_cost + filter_cost);
}

double IndexAwareOptimizer::estimate_index_scan_cost(const IndexSelectionStats& stats,
                                                    const std::string& index_type) {
    double base_cost;
    
    if (index_type == "BTree") {
        // O(log N) for tree traversal
        double log_n = std::log2(stats.row_count + 1);
        double access_cost = BTREE_ACCESS_COST * log_n;
        
        // Fetch result rows: O(K) where K = result set size
        long long result_rows = static_cast<long long>(stats.row_count * stats.selectivity);
        double fetch_cost = static_cast<double>(result_rows) / 1000000.0;
        
        base_cost = access_cost + fetch_cost;
    } 
    else if (index_type == "Hash") {
        // O(1) hash table lookup
        double access_cost = HASH_ACCESS_COST;
        
        // Fetch result rows
        long long result_rows = static_cast<long long>(stats.row_count * stats.selectivity);
        double fetch_cost = static_cast<double>(result_rows) / 1000000.0;
        
        base_cost = access_cost + fetch_cost;
    } 
    else if (index_type == "Bitmap") {
        // Bitmap is very efficient for low cardinality
        if (stats.cardinality > 0 && stats.cardinality <= 100) {
            // For low cardinality, bitmap is extremely fast
            base_cost = BITMAP_ACCESS_COST * std::log2(stats.cardinality + 1);
        } 
        else if (stats.cardinality <= 1000) {
            // Medium cardinality, still good
            base_cost = BITMAP_ACCESS_COST * 1.5;
        } 
        else {
            // High cardinality, not suitable for bitmap
            // Fall back to full scan cost
            return estimate_scan_cost(stats);
        }
    } 
    else {
        return estimate_scan_cost(stats);
    }

    return base_cost;
}

void IndexAwareOptimizer::detect_index_opportunities(
    PlanNode* node,
    std::vector<IndexSelectionStats>& opportunities) {
    
    if (!node) return;

    // Simplified: just traverse the tree without IndexManager calls
    // Full implementation would detect filter nodes and propose indexes
    
    auto children = node->children();
    for (auto child : children) {
        detect_index_opportunities(child, opportunities);
    }
}

bool IndexAwareOptimizer::extract_column_and_value(
    const std::string& condition,
    std::string& column,
    std::string& value) {
    
    // Pattern 1: column = 'value'
    std::regex quoted_regex(R"((\w+)\s*=\s*'([^']*)')");
    std::smatch match;
    
    if (std::regex_search(condition, match, quoted_regex)) {
        column = match[1].str();
        value = match[2].str();
        return true;
    }
    
    // Pattern 2: column = value (unquoted)
    std::regex unquoted_regex(R"((\w+)\s*=\s*(\w+))");
    if (std::regex_search(condition, match, unquoted_regex)) {
        column = match[1].str();
        value = match[2].str();
        return true;
    }
    
    return false;
}

bool IndexAwareOptimizer::is_equality_predicate(const std::string& condition) {
    // Must have = but not !=, <>, <=, >=
    bool has_equals = condition.find("=") != std::string::npos;
    bool has_not_equal = condition.find("!=") != std::string::npos;
    bool has_angle = condition.find("<>") != std::string::npos;
    bool has_le = condition.find("<=") != std::string::npos;
    bool has_ge = condition.find(">=") != std::string::npos;
    
    return has_equals && !has_not_equal && !has_angle && !has_le && !has_ge;
}

bool IndexAwareOptimizer::is_range_predicate(const std::string& condition) {
    // Range: <, >, <=, >= but not IN or BETWEEN
    bool has_comparison = (condition.find("<") != std::string::npos ||
                           condition.find(">") != std::string::npos);
    bool has_in = condition.find("IN") != std::string::npos;
    bool has_between = condition.find("BETWEEN") != std::string::npos;
    
    return has_comparison && !has_in && !has_between;
}

bool IndexAwareOptimizer::is_in_list_predicate(const std::string& condition) {
    // Must have IN but not BETWEEN
    bool has_in = condition.find("IN") != std::string::npos;
    bool has_between = condition.find("BETWEEN") != std::string::npos;
    
    return has_in && !has_between;
}

} // namespace plan
} // namespace lyradb
