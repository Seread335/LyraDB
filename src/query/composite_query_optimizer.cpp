/**
 * @file composite_query_optimizer.cpp
 * @brief Implementation of composite query optimizer
 * 
 * Phase 4.2: B-Tree Query Optimization
 */

#include "lyradb/composite_query_optimizer.h"
#include "lyradb/range_query_optimizer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace lyradb {

// ============================================================================
// Helper Functions
// ============================================================================

static std::string extract_primary_column(const std::string& where_clause) {
    // Simple extraction: find the first identifier before an operator
    std::string column;
    
    for (size_t i = 0; i < where_clause.length(); ++i) {
        char c = where_clause[i];
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_') || 
            (i > 0 && c >= '0' && c <= '9')) {
            column += c;
        } else if (!column.empty()) {
            // Found operator, return column
            return column;
        }
    }
    
    return column;
}

// ============================================================================
// Constructor/Destructor
// ============================================================================

CompositeQueryOptimizer::CompositeQueryOptimizer()
    : range_optimizer_(std::make_unique<query::RangeQueryOptimizer>()) {
}

CompositeQueryOptimizer::~CompositeQueryOptimizer() = default;

// ============================================================================
// OptimizationDecision Implementation
// ============================================================================

std::string CompositeQueryOptimizer::OptimizationDecision::to_string() const {
    std::ostringstream oss;
    
    oss << "Optimization Decision:\n";
    oss << "  Use Index: " << (use_index ? "Yes" : "No") << "\n";
    
    if (use_index) {
        oss << "  Primary Index: " << primary_index << "\n";
        oss << "  Estimated Selectivity: " << (estimated_selectivity * 100) << "%\n";
        oss << "  Estimated Speedup: " << estimated_speedup << "x\n";
    }
    
    if (!reason.empty()) {
        oss << "  Reason: " << reason << "\n";
    }
    
    return oss.str();
}

// ============================================================================
// OptimizationStats Implementation
// ============================================================================

std::string CompositeQueryOptimizer::OptimizationStats::to_string() const {
    std::ostringstream oss;
    
    oss << "Optimization Statistics:\n";
    oss << "  Queries Analyzed: " << queries_analyzed << "\n";
    oss << "  Queries Optimized: " << queries_optimized << "\n";
    oss << "  Range Predicates Found: " << range_predicates_found << "\n";
    oss << "  Indexes Recommended: " << indexes_recommended << "\n";
    
    if (queries_optimized > 0) {
        oss << "  Average Selectivity: " << (avg_selectivity * 100) << "%\n";
        oss << "  Total Speedup (sum): " << total_estimated_speedup << "x\n";
    }
    
    return oss.str();
}

// ============================================================================
// analyze_query Implementation
// ============================================================================

CompositeQueryOptimizer::OptimizationDecision CompositeQueryOptimizer::analyze_query(
    const std::string& table_name,
    const std::string& where_clause,
    size_t table_size,
    const std::vector<std::string>& available_indexes) {
    
    OptimizationDecision decision;
    stats_.queries_analyzed++;
    
    // Handle empty WHERE clause
    if (where_clause.empty()) {
        decision.reason = "No WHERE clause - full table scan";
        return decision;
    }
    
    // Detect multiple predicates for composite index optimization
    bool has_and = where_clause.find(" AND ") != std::string::npos;
    bool has_or = where_clause.find(" OR ") != std::string::npos;
    
    // Detect range predicates from the WHERE clause string
    // Check for range operators: >, <, >=, <=, BETWEEN
    bool has_range_predicate = (where_clause.find(">") != std::string::npos ||
                                where_clause.find("<") != std::string::npos ||
                                where_clause.find("BETWEEN") != std::string::npos);
    
    if (!has_range_predicate) {
        decision.reason = "No range predicates detected";
        return decision;
    }
    
    stats_.range_predicates_found++;
    
    // Check table size threshold
    if (table_size < MIN_TABLE_SIZE) {
        decision.reason = "Table size (" + std::to_string(table_size) + 
                         ") below threshold (" + std::to_string(MIN_TABLE_SIZE) + ")";
        return decision;
    }
    
    // Estimate selectivity
    decision.estimated_selectivity = estimate_selectivity_from_clause(where_clause, table_size);
    
    // Check selectivity threshold
    if (decision.estimated_selectivity > SELECTIVITY_THRESHOLD) {
        decision.reason = "Selectivity (" + std::to_string(decision.estimated_selectivity * 100) +
                         "%) above threshold (" + std::to_string(SELECTIVITY_THRESHOLD * 100) + "%)";
        return decision;
    }
    
    // Calculate estimated speedup
    decision.estimated_speedup = estimate_speedup(
        decision.estimated_selectivity,
        table_size
    );
    
    // Check minimum speedup requirement
    if (decision.estimated_speedup < MIN_SPEEDUP) {
        decision.reason = "Estimated speedup (" + std::to_string(decision.estimated_speedup) +
                         "x) below threshold (" + std::to_string(MIN_SPEEDUP) + "x)";
        return decision;
    }
    
    // Find best index for primary predicate (single or composite)
    // Check if this is a composite query (AND clauses)
    std::string primary_column = extract_primary_column(where_clause);
    
    if (!primary_column.empty()) {
        // Check for composite predicates
        if (has_and && available_indexes.empty()) {
            // Multiple conditions - recommend composite index
            decision.use_multiple_indexes = true;
            decision.primary_index = "idx_" + table_name + "_composite";
            
            // For composite indexes, selectivity multiplies
            // WHERE a = 'x' (1% sel) AND b > 100 (50% sel) = 0.5% combined
            decision.estimated_selectivity *= 0.5;  // Rough composite estimate
            
            // Recalculate speedup for lower selectivity
            decision.estimated_speedup = estimate_speedup(
                decision.estimated_selectivity,
                table_size
            );
        } else {
            // Simple single-column index
            decision.primary_index = "idx_" + table_name + "_" + primary_column;
        }
        
        decision.use_index = true;
        decision.indexes.push_back(decision.primary_index);
        
        stats_.queries_optimized++;
        stats_.indexes_recommended++;
        stats_.total_estimated_speedup += decision.estimated_speedup;
        stats_.avg_selectivity = (stats_.avg_selectivity * (stats_.queries_optimized - 1) +
                                 decision.estimated_selectivity) / stats_.queries_optimized;
        
        std::string index_type = decision.use_multiple_indexes ? "composite" : "single-column";
        decision.reason = "B-tree " + index_type + " index recommended for column '" + primary_column + 
                         "'. Estimated " + std::to_string(decision.estimated_speedup) + "x speedup.";
        
        if (verbose_) {
            std::cout << "Query on table '" << table_name << "' optimized:\n"
                     << decision.to_string() << "\n";
        }
    }
    
    return decision;
}

// ============================================================================
// optimize_plan Implementation
// ============================================================================

std::shared_ptr<plan::PlanNode> CompositeQueryOptimizer::optimize_plan(
    std::shared_ptr<plan::PlanNode> plan,
    size_t table_size,
    const std::vector<std::string>& available_indexes) {
    
    // In a full implementation, this would traverse the plan tree
    // and replace appropriate ScanNode + FilterNode combinations
    // with IndexedScanNode using B-tree indexes
    
    // For now, return original plan
    // (This would be integrated with index_aware_optimizer)
    
    return plan;
}

// ============================================================================
// recommend_index_creation Implementation
// ============================================================================

bool CompositeQueryOptimizer::recommend_index_creation(
    const std::string& column_name,
    size_t table_size,
    double estimated_selectivity) {
    
    // Don't index small tables
    if (table_size < MIN_TABLE_SIZE) {
        return false;
    }
    
    // Index if selectivity is low enough
    if (estimated_selectivity >= SELECTIVITY_THRESHOLD) {
        return false;
    }
    
    // Calculate estimated speedup
    double speedup = estimate_speedup(estimated_selectivity, table_size);
    
    // Recommend if speedup is significant
    return speedup >= MIN_SPEEDUP;
}

// ============================================================================
// get_stats Implementation
// ============================================================================

const CompositeQueryOptimizer::OptimizationStats& CompositeQueryOptimizer::get_stats() const {
    return stats_;
}

// ============================================================================
// reset_stats Implementation
// ============================================================================

void CompositeQueryOptimizer::reset_stats() {
    stats_ = OptimizationStats();
}

// ============================================================================
// estimate_selectivity_from_clause Implementation
// ============================================================================

double CompositeQueryOptimizer::estimate_selectivity_from_clause(
    const std::string& where_clause,
    size_t table_size) {
    
    if (where_clause.empty()) {
        return 1.0;
    }
    
    // PHASE 4.4 FIX: More conservative selectivity estimation
    // Old model was too optimistic, especially for:
    // - Range predicates (> and <)
    // - AND predicates (multiplicative overhead not counted)
    // - Real-world query patterns
    
    double combined_selectivity = 1.0;
    int predicate_count = 0;
    int and_count = 0;
    int or_count = 0;
    
    // Count different operator types and estimate individual selectivity
    
    // Equality predicates: ~1-5% selectivity depending on cardinality
    int eq_count = 0;
    size_t pos = 0;
    while ((pos = where_clause.find("=", pos)) != std::string::npos) {
        if (pos > 0 && where_clause[pos-1] != '<' && where_clause[pos-1] != '>' && 
            where_clause[pos-1] != '=') {
            if (pos + 1 < where_clause.length() && where_clause[pos+1] != '=') {
                eq_count++;
                // Conservative: equality typically 1-2% selectivity
                combined_selectivity *= 0.015;
                predicate_count++;
            }
        }
        pos += 1;
    }
    
    // Range predicates (>, <, >=, <=): TYPICALLY 30-75% selectivity
    // NOT 20-50% as old model thought!
    // This is the biggest source of wrong predictions in Phase 4.4
    bool has_greater = where_clause.find(">") != std::string::npos;
    bool has_less = where_clause.find("<") != std::string::npos;
    
    if (has_greater) {
        // "age > X" typically matches ~75% of rows for age distribution
        // NOT 30% as old model estimated
        combined_selectivity *= 0.75;
        predicate_count++;
    }
    if (has_less) {
        // "age < X" or similar: ~30-40% selectivity
        combined_selectivity *= 0.35;
        predicate_count++;
    }
    
    // BETWEEN predicates: ~10-20% selectivity
    if (where_clause.find("BETWEEN") != std::string::npos) {
        combined_selectivity *= 0.15;
        predicate_count++;
    }
    
    // AND operators: multiplicative selectivity BUT with overhead penalty
    pos = 0;
    while ((pos = where_clause.find(" AND ", pos)) != std::string::npos) {
        and_count++;
        pos += 5;
    }
    
    // OR operators: additive selectivity (bad for optimization)
    pos = 0;
    while ((pos = where_clause.find(" OR ", pos)) != std::string::npos) {
        or_count++;
        pos += 4;
    }
    
    // If no predicates found, use high selectivity (full scan likely better)
    if (predicate_count == 0) {
        return 0.75;  // Conservative: assume most rows match
    }
    
    // For AND clauses with multiple predicates, add execution overhead
    // 2 AND predicates: still apply multiplication but add overhead cost
    // 3+ AND predicates: even higher overhead penalty
    if (and_count > 0 && or_count == 0) {
        // Pure AND: multiplicative selectivity
        // BUT: add overhead penalty for multiple index operations
        double and_overhead = 1.0 + (and_count * 0.1);  // 10% overhead per additional AND
        combined_selectivity = std::min(1.0, combined_selectivity * and_overhead);
    }
    else if (or_count > 0) {
        // OR present: very conservative (indexes don't help much with OR)
        // OR makes selectivity higher (more rows match)
        combined_selectivity = std::min(1.0, combined_selectivity + (0.15 * or_count));
    }
    
    // Clamp to valid range [0.0, 1.0]
    return std::max(0.0, std::min(1.0, combined_selectivity));
}

// ============================================================================
// should_use_index Implementation
// ============================================================================

bool CompositeQueryOptimizer::should_use_index(double selectivity, size_t table_size) {
    // PHASE 4.4 FIX: More conservative thresholds
    // Old model: SELECTIVITY_THRESHOLD = 0.25 (25%)
    // Real data: Indexes don't help much above 15%
    
    // Don't use index if selectivity is too high
    // For range predicates > 50%, index is almost always slower
    if (selectivity > 0.15) {
        // THRESHOLD RAISED: was 0.25, now 0.15
        // Only use index for highly selective queries
        return false;
    }
    // Don't use index if table is too small
    if (table_size < MIN_TABLE_SIZE) {
        return false;
    }
    
    // Check if speedup is significant
    double speedup = estimate_speedup(selectivity, table_size);
    return speedup >= MIN_SPEEDUP;
}

// ============================================================================
// estimate_speedup Implementation
// ============================================================================

double CompositeQueryOptimizer::estimate_speedup(double selectivity, size_t table_size) {
    // PHASE 4.4 FIX: Conservative cost model that matches real-world performance
    // 
    // Issues with old model:
    // 1. Linear formulas didn't account for cache effects
    // 2. Selectivity thresholds were too aggressive
    // 3. Didn't factor in index overhead properly for AND predicates
    // 4. Range predicates (>50% selectivity) should almost never use index
    //
    // Performance model:
    // B-tree: O(log n) + O(k) where k = selected rows
    // Full scan: O(n)
    // Real overhead: 10-30% of log(n) cost for tree traversal
    
    // CRITICAL: If selectivity > 50%, index rarely helps
    if (selectivity >= 0.5) {
        return 0.9;  // Conservative: maybe slight overhead from index
    }
    
    // For selectivity > 30%, index helps but not much
    if (selectivity >= 0.3) {
        if (table_size >= 100000) {
            return 1.2;  // Modest gain on very large tables
        }
        return 1.0;  // Break even on smaller tables
    }
    
    if (table_size < 100) {
        return 0.8;  // Overhead dominates on tiny tables
    }
    
    // Calculate realistic speedup based on selectivity sweet spot
    double log_n = std::log(static_cast<double>(table_size)) / std::log(2.0);
    
    // For selectivity < 30%, index can help significantly
    // But still much more conservative than old model
    
    if (table_size < 1000) {
        // Small tables: 1-2x speedup ONLY for very selective queries (< 5%)
        if (selectivity < 0.05) {
            return 1.5;
        } else if (selectivity < 0.1) {
            return 1.2;
        }
        return 1.0;
    }
    else if (table_size < 10000) {
        // Medium tables
        if (selectivity < 0.05) {
            return 2.0;
        } else if (selectivity < 0.1) {
            return 1.5;
        } else if (selectivity < 0.2) {
            return 1.2;
        }
        return 1.0;
    }
    else if (table_size < 100000) {
        // Larger tables: better selectivity gains
        if (selectivity < 0.01) {
            return 3.0;  // Very selective: up to 3x
        } else if (selectivity < 0.05) {
            return 2.5;
        } else if (selectivity < 0.1) {
            return 2.0;
        } else if (selectivity < 0.2) {
            return 1.5;
        }
        return 1.1;
    }
    else {
        // Very large tables: best use of indexes
        if (selectivity < 0.01) {
            return 4.0;  // Highly selective: up to 4x
        } else if (selectivity < 0.05) {
            return 3.0;
        } else if (selectivity < 0.1) {
            return 2.5;
        } else if (selectivity < 0.2) {
            return 2.0;
        } else if (selectivity < 0.3) {
            return 1.5;
        }
        return 1.1;
    }
}

// ============================================================================
// Cost-Based Query Planning Implementation
// ============================================================================

double CompositeQueryOptimizer::calculate_scan_cost(size_t table_size) const {
    // Full table scan cost model:
    // Cost = table_size (must read every row)
    // + overhead factor for cache misses on large tables
    
    if (table_size < 1000) {
        // Small tables: negligible overhead
        return static_cast<double>(table_size);
    }
    
    // Add cache miss overhead for larger tables
    // Assumption: CPU cache ~ 64KB, L3 cache ~ 8MB
    // Cache miss cost ~100x CPU cycle cost
    double cache_miss_factor = 1.0 + (std::log(static_cast<double>(table_size)) / 30.0);
    
    return static_cast<double>(table_size) * cache_miss_factor;
}

double CompositeQueryOptimizer::calculate_index_cost(
    size_t table_size,
    double selectivity) const {
    
    // B-tree indexed scan cost model:
    // Cost = log2(table_size) (B-tree traversal)
    //        + (selectivity * table_size) (result scanning and processing)
    //        + index_overhead (cache line misses, pointer dereferences)
    
    if (table_size < 100) {
        // Very small tables: full scan is actually cheaper
        return static_cast<double>(table_size);
    }
    
    // Calculate components
    double log_cost = std::log2(static_cast<double>(table_size));
    double result_cost = selectivity * static_cast<double>(table_size);
    
    // Index overhead: 20% for small tables, 5-10% for large tables
    double overhead_factor = 1.15;
    if (table_size > 10000) {
        overhead_factor = 1.05;
    }
    else if (table_size > 100000) {
        overhead_factor = 1.02;
    }
    
    return (log_cost + result_cost) * overhead_factor;
}

std::string CompositeQueryOptimizer::choose_best_index(
    const std::vector<std::string>& available_indexes,
    double selectivity,
    size_t table_size) const {
    
    // Cost-based index selection
    if (available_indexes.empty()) {
        return "";  // No index available
    }
    
    // Calculate costs
    double scan_cost = calculate_scan_cost(table_size);
    double index_cost = calculate_index_cost(table_size, selectivity);
    
    // If scan is cheaper, don't use any index
    if (scan_cost <= index_cost) {
        return "";
    }
    
    // For now, prefer composite index if available
    for (const auto& idx : available_indexes) {
        if (idx.find("composite") != std::string::npos) {
            return idx;
        }
    }
    
    // Otherwise, return first available index
    return available_indexes.front();
}

}  // namespace lyradb
