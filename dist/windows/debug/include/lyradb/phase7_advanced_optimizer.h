// PHASE 7: Advanced Optimization Pipeline Integration
// Wraps and fixes Phase 4.4 modules without breaking existing code
// Provides clean API for CompositeIndexOptimizer, QueryRewriter, IndexAdvisor

#pragma once

#include <string>
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <optional>

namespace lyradb {
namespace phase7 {

/**
 * PHASE 7: Advanced Query Optimization
 * 
 * Bridges Phase 4.4 modules (CompositeIndexOptimizer, QueryRewriter, IndexAdvisor)
 * with Phase 6 (SimpleQueryOptimizer) for a complete optimization pipeline.
 * 
 * Architecture:
 * 1. Predicate analysis (break down WHERE clauses)
 * 2. Index availability check
 * 3. Strategy selection (composite, intersection, union, scan)
 * 4. Cost estimation
 * 5. Query rewriting
 * 6. Index advisor recommendations
 */

/**
 * Predicate information - parsed from WHERE clause
 */
struct Predicate {
    std::string column;
    std::string op;              // =, <, >, <=, >=, !=, IN, BETWEEN, LIKE
    std::string value;
    std::string logical_op;      // AND, OR
    double estimated_selectivity;
    
    Predicate() : logical_op("AND"), estimated_selectivity(0.5) {}
};

/**
 * Query optimization plan from Phase 7
 */
struct OptimizationPlan {
    enum class Strategy {
        FULL_SCAN,               // O(n) - no optimization
        INDEX_SINGLE,            // O(log n) - single index lookup
        INDEX_RANGE,             // O(log n + k) - single index range
        INDEX_COMPOSITE,         // O(log n) - multi-column B-tree
        INDEX_INTERSECTION,      // O(n + m) - AND predicates
        INDEX_UNION,             // O(n + m) - OR predicates
        INDEX_HYBRID             // Complex: mix of operations
    };
    
    Strategy strategy;
    std::vector<std::string> indexes_used;
    double estimated_speedup;
    size_t estimated_rows;
    std::string execution_plan;
    std::string cost_breakdown;
    
    OptimizationPlan() 
        : strategy(Strategy::FULL_SCAN), 
          estimated_speedup(1.0), 
          estimated_rows(0) {}
};

/**
 * Index recommendation from Phase 7 advisor
 */
struct IndexRecommendation {
    std::string index_name;
    std::vector<std::string> columns;
    std::string reason;
    double estimated_improvement;
    
    IndexRecommendation() : estimated_improvement(0.0) {}
};

/**
 * PHASE 7: Advanced Optimizer
 * Orchestrates Phase 4.4 modules cleanly
 */
class AdvancedOptimizer {
private:
    std::vector<std::string> available_indexes;
    std::map<std::string, std::vector<std::string>> composite_indexes;
    size_t table_size;
    
    // Helper methods for Phase 4.4 integration
    bool analyze_predicates(const std::vector<Predicate>& predicates);
    bool check_composite_index_availability(const std::vector<std::string>& columns);
    double calculate_selectivity(const Predicate& pred);
    double calculate_combined_selectivity(const std::vector<Predicate>& preds, bool is_and);
    std::string generate_execution_plan(const std::vector<Predicate>& preds, 
                                       const OptimizationPlan::Strategy& strategy);
    
public:
    AdvancedOptimizer() : table_size(0) {}
    
    /**
     * Register available indexes on the table
     */
    void register_index(const std::string& index_name, 
                       const std::vector<std::string>& columns = {}) {
        available_indexes.push_back(index_name);
        if (!columns.empty()) {
            composite_indexes[index_name] = columns;
        }
    }
    
    /**
     * Set table size for cost estimation
     */
    void set_table_size(size_t rows) {
        table_size = rows;
    }
    
    /**
     * Parse WHERE clause into predicates
     * Format: "column op value [AND/OR column op value]*"
     * Examples:
     *   "age > 18"
     *   "age = 25 AND country = USA"
     *   "status = active OR status = pending"
     */
    std::vector<Predicate> parse_where_clause(const std::string& where_clause);
    
    /**
     * Generate optimization plan for multi-predicate query
     */
    OptimizationPlan optimize(const std::vector<Predicate>& predicates);
    
    /**
     * Generate optimization plan from WHERE clause string
     */
    OptimizationPlan optimize_where(const std::string& where_clause) {
        auto predicates = parse_where_clause(where_clause);
        return optimize(predicates);
    }
    
    /**
     * Provide index recommendations for query optimization
     */
    std::vector<IndexRecommendation> get_recommendations(
        const std::vector<Predicate>& predicates);
    
    /**
     * Rewrite query using optimization plan (Phase 4.4 QueryRewriter)
     */
    std::string rewrite_query(const std::string& original_query, 
                             const OptimizationPlan& plan);
    
    /**
     * Get statistics about optimization
     */
    std::string get_stats() const;
};

// Implementation

std::vector<Predicate> AdvancedOptimizer::parse_where_clause(const std::string& where_clause) {
    std::vector<Predicate> predicates;
    
    if (where_clause.empty()) {
        return predicates;
    }
    
    // Simple parser for: "column op value [AND/OR column op value]*"
    size_t pos = 0;
    std::string current_logical_op = "AND";
    
    while (pos < where_clause.length()) {
        Predicate pred;
        pred.logical_op = current_logical_op;
        
        // Skip whitespace
        while (pos < where_clause.length() && std::isspace(where_clause[pos])) {
            pos++;
        }
        
        // Extract column name
        size_t col_start = pos;
        while (pos < where_clause.length() && 
               (std::isalnum(where_clause[pos]) || where_clause[pos] == '_')) {
            pos++;
        }
        
        if (col_start == pos) break;  // No column found
        
        pred.column = where_clause.substr(col_start, pos - col_start);
        
        // Skip whitespace and extract operator
        while (pos < where_clause.length() && std::isspace(where_clause[pos])) {
            pos++;
        }
        
        if (pos >= where_clause.length()) break;
        
        // Extract operator
        size_t op_start = pos;
        if (where_clause[pos] == '>' || where_clause[pos] == '<' || 
            where_clause[pos] == '=' || where_clause[pos] == '!') {
            pos++;
            if (pos < where_clause.length() && where_clause[pos] == '=') {
                pos++;
            }
        }
        
        if (op_start == pos) break;  // No operator
        
        pred.op = where_clause.substr(op_start, pos - op_start);
        
        // Skip whitespace and extract value
        while (pos < where_clause.length() && std::isspace(where_clause[pos])) {
            pos++;
        }
        
        size_t val_start = pos;
        while (pos < where_clause.length() && 
               where_clause[pos] != 'A' && where_clause[pos] != 'O' &&
               !std::isspace(where_clause[pos])) {
            pos++;
        }
        
        if (val_start == pos) break;  // No value
        
        pred.value = where_clause.substr(val_start, pos - val_start);
        
        // Calculate estimated selectivity based on operator
        if (pred.op == "=") {
            pred.estimated_selectivity = 0.1;  // Equality is selective
        } else if (pred.op == "<" || pred.op == ">" || 
                   pred.op == "<=" || pred.op == ">=") {
            pred.estimated_selectivity = 0.33;  // Range is moderately selective
        } else {
            pred.estimated_selectivity = 0.5;   // Unknown - conservative estimate
        }
        
        predicates.push_back(pred);
        
        // Check for AND/OR
        while (pos < where_clause.length() && std::isspace(where_clause[pos])) {
            pos++;
        }
        
        if (pos + 2 < where_clause.length()) {
            if ((where_clause[pos] == 'A' || where_clause[pos] == 'a') &&
                (where_clause[pos+1] == 'N' || where_clause[pos+1] == 'n') &&
                (where_clause[pos+2] == 'D' || where_clause[pos+2] == 'd')) {
                current_logical_op = "AND";
                pos += 3;
            } else if ((where_clause[pos] == 'O' || where_clause[pos] == 'o') &&
                       (where_clause[pos+1] == 'R' || where_clause[pos+1] == 'r')) {
                current_logical_op = "OR";
                pos += 2;
            }
        }
    }
    
    return predicates;
}

double AdvancedOptimizer::calculate_selectivity(const Predicate& pred) {
    return pred.estimated_selectivity;
}

double AdvancedOptimizer::calculate_combined_selectivity(
    const std::vector<Predicate>& preds, bool is_and) {
    
    if (preds.empty()) return 1.0;
    
    double selectivity = 1.0;
    
    for (const auto& pred : preds) {
        if (is_and) {
            selectivity *= calculate_selectivity(pred);
        } else {
            selectivity = 1.0 - (1.0 - selectivity) * (1.0 - calculate_selectivity(pred));
        }
    }
    
    return selectivity;
}

bool AdvancedOptimizer::check_composite_index_availability(
    const std::vector<std::string>& columns) {
    
    for (const auto& [index_name, index_cols] : composite_indexes) {
        if (index_cols.size() >= columns.size()) {
            bool matches = true;
            for (size_t i = 0; i < columns.size(); ++i) {
                if (index_cols[i] != columns[i]) {
                    matches = false;
                    break;
                }
            }
            if (matches) return true;
        }
    }
    return false;
}

std::string AdvancedOptimizer::generate_execution_plan(
    const std::vector<Predicate>& preds,
    const OptimizationPlan::Strategy& strategy) {
    
    std::string plan = "Execution Plan:\n";
    
    if (strategy == OptimizationPlan::Strategy::FULL_SCAN) {
        plan += "  1. TableScan: Read all " + std::to_string(table_size) + " rows\n";
        plan += "  2. Filter: Apply predicates\n";
    }
    else if (strategy == OptimizationPlan::Strategy::INDEX_SINGLE) {
        plan += "  1. IndexLookup: " + preds[0].column + " " + preds[0].op + " " + preds[0].value + "\n";
    }
    else if (strategy == OptimizationPlan::Strategy::INDEX_RANGE) {
        plan += "  1. IndexRangeScan: " + preds[0].column + " " + preds[0].op + " " + preds[0].value + "\n";
    }
    else if (strategy == OptimizationPlan::Strategy::INDEX_COMPOSITE) {
        plan += "  1. CompositeIndexLookup: ";
        for (size_t i = 0; i < preds.size(); ++i) {
            if (i > 0) plan += " AND ";
            plan += preds[i].column + " " + preds[i].op + " " + preds[i].value;
        }
        plan += "\n";
    }
    else if (strategy == OptimizationPlan::Strategy::INDEX_INTERSECTION) {
        plan += "  1. MultiIndexLookup:\n";
        for (size_t i = 0; i < preds.size(); ++i) {
            plan += "     - Index on " + preds[i].column + ": " + preds[i].op + " " + preds[i].value + "\n";
        }
        plan += "  2. SetIntersection: Combine results\n";
    }
    else if (strategy == OptimizationPlan::Strategy::INDEX_UNION) {
        plan += "  1. MultiIndexLookup:\n";
        for (size_t i = 0; i < preds.size(); ++i) {
            plan += "     - Index on " + preds[i].column + ": " + preds[i].op + " " + preds[i].value + "\n";
        }
        plan += "  2. SetUnion: Combine results\n";
    }
    
    return plan;
}

OptimizationPlan AdvancedOptimizer::optimize(const std::vector<Predicate>& predicates) {
    OptimizationPlan plan;
    
    if (predicates.empty() || table_size == 0) {
        plan.strategy = OptimizationPlan::Strategy::FULL_SCAN;
        plan.estimated_speedup = 1.0;
        plan.estimated_rows = table_size;
        return plan;
    }
    
    // Determine if all predicates are AND or OR
    bool all_and = true;
    for (const auto& pred : predicates) {
        if (pred.logical_op == "OR") {
            all_and = false;
            break;
        }
    }
    
    // Single predicate cases
    if (predicates.size() == 1) {
        const auto& pred = predicates[0];
        
        // Check if index exists
        bool has_index = false;
        for (const auto& idx : available_indexes) {
            if (idx.find(pred.column) != std::string::npos) {
                has_index = true;
                plan.indexes_used.push_back(idx);
                break;
            }
        }
        
        if (has_index) {
            if (pred.op == "=") {
                plan.strategy = OptimizationPlan::Strategy::INDEX_SINGLE;
                plan.estimated_speedup = 100.0;  // O(log n) vs O(n)
            } else {
                plan.strategy = OptimizationPlan::Strategy::INDEX_RANGE;
                plan.estimated_speedup = 50.0;   // O(log n + k)
            }
        } else {
            plan.strategy = OptimizationPlan::Strategy::FULL_SCAN;
            plan.estimated_speedup = 1.0;
        }
    }
    // Multiple predicates
    else {
        // Check for composite index
        std::vector<std::string> columns;
        for (const auto& pred : predicates) {
            columns.push_back(pred.column);
        }
        
        if (check_composite_index_availability(columns)) {
            plan.strategy = OptimizationPlan::Strategy::INDEX_COMPOSITE;
            plan.estimated_speedup = 100.0;
            plan.indexes_used.push_back("composite_index");
        }
        else if (all_and) {
            // Check if all columns have indexes
            bool all_indexed = true;
            for (const auto& pred : predicates) {
                bool found = false;
                for (const auto& idx : available_indexes) {
                    if (idx.find(pred.column) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    all_indexed = false;
                    break;
                }
            }
            
            if (all_indexed) {
                plan.strategy = OptimizationPlan::Strategy::INDEX_INTERSECTION;
                plan.estimated_speedup = 20.0;  // O(n + m) vs O(n*m)
                for (const auto& pred : predicates) {
                    plan.indexes_used.push_back("idx_" + pred.column);
                }
            } else {
                plan.strategy = OptimizationPlan::Strategy::FULL_SCAN;
                plan.estimated_speedup = 1.0;
            }
        }
        else {
            // OR predicates
            bool all_indexed = true;
            for (const auto& pred : predicates) {
                bool found = false;
                for (const auto& idx : available_indexes) {
                    if (idx.find(pred.column) != std::string::npos) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    all_indexed = false;
                    break;
                }
            }
            
            if (all_indexed) {
                plan.strategy = OptimizationPlan::Strategy::INDEX_UNION;
                plan.estimated_speedup = 10.0;  // O(n + m) vs O(n)
                for (const auto& pred : predicates) {
                    plan.indexes_used.push_back("idx_" + pred.column);
                }
            } else {
                plan.strategy = OptimizationPlan::Strategy::FULL_SCAN;
                plan.estimated_speedup = 1.0;
            }
        }
    }
    
    // Estimate result rows
    double selectivity = calculate_combined_selectivity(
        predicates, 
        all_and
    );
    plan.estimated_rows = static_cast<size_t>(table_size * selectivity);
    
    // Generate execution plan
    plan.execution_plan = generate_execution_plan(predicates, plan.strategy);
    
    // Cost breakdown
    plan.cost_breakdown = "Estimated cost: " + 
        std::to_string(static_cast<int>(plan.estimated_speedup)) + 
        "x faster than full scan\n";
    plan.cost_breakdown += "Estimated result rows: " + std::to_string(plan.estimated_rows) + "\n";
    
    return plan;
}

std::vector<IndexRecommendation> AdvancedOptimizer::get_recommendations(
    const std::vector<Predicate>& predicates) {
    
    std::vector<IndexRecommendation> recommendations;
    
    for (const auto& pred : predicates) {
        bool has_index = false;
        for (const auto& idx : available_indexes) {
            if (idx.find(pred.column) != std::string::npos) {
                has_index = true;
                break;
            }
        }
        
        if (!has_index) {
            IndexRecommendation rec;
            rec.index_name = "idx_" + pred.column;
            rec.columns.push_back(pred.column);
            rec.reason = "Missing index on " + pred.column + " for predicate optimization";
            rec.estimated_improvement = 50.0;  // Estimated 50x improvement
            recommendations.push_back(rec);
        }
    }
    
    return recommendations;
}

std::string AdvancedOptimizer::rewrite_query(const std::string& original_query,
                                            const OptimizationPlan& plan) {
    std::string rewritten = original_query;
    
    // QueryRewriter (Phase 4.4) logic
    // For now, simple hint-based rewriting
    
    if (plan.strategy == OptimizationPlan::Strategy::INDEX_COMPOSITE ||
        plan.strategy == OptimizationPlan::Strategy::INDEX_INTERSECTION) {
        
        // Add USE INDEX hint
        size_t from_pos = rewritten.find(" FROM ");
        if (from_pos != std::string::npos) {
            size_t table_start = from_pos + 6;
            size_t table_end = table_start;
            while (table_end < rewritten.length() && 
                   (std::isalnum(rewritten[table_end]) || rewritten[table_end] == '_')) {
                table_end++;
            }
            
            std::string table_name = rewritten.substr(table_start, table_end - table_start);
            
            // Insert USE INDEX hint after table name
            std::string hint = " USE INDEX (";
            for (size_t i = 0; i < plan.indexes_used.size(); ++i) {
                if (i > 0) hint += ", ";
                hint += plan.indexes_used[i];
            }
            hint += ")";
            
            rewritten.insert(table_end, hint);
        }
    }
    
    return rewritten;
}

std::string AdvancedOptimizer::get_stats() const {
    std::string stats;
    stats += "Advanced Optimizer Stats:\n";
    stats += "  Available indexes: " + std::to_string(available_indexes.size()) + "\n";
    stats += "  Table size: " + std::to_string(table_size) + " rows\n";
    return stats;
}

} // namespace phase7
} // namespace lyradb
