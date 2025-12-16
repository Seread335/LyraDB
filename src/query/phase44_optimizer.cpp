#include "lyradb/phase44_optimizer.h"
#include <sstream>
#include <algorithm>
#include <cmath>

namespace lyradb {
namespace integration {

Phase44QueryOptimizer::Phase44QueryOptimizer()
    : total_queries_optimized_(0), total_speedup_(0.0) {
}

Phase44QueryOptimizer::~Phase44QueryOptimizer() {
}

Phase44QueryOptimizer::QueryOptimizationPlan Phase44QueryOptimizer::optimize_where_clause(
    const std::string& where_clause,
    size_t table_size,
    const std::vector<std::string>& available_indexes) {
    
    QueryOptimizationPlan plan;
    plan.query_text = where_clause;
    
    // Parse the WHERE clause
    auto predicates = parse_where_clause(where_clause);
    
    if (predicates.empty()) {
        plan.strategy = "full_scan";
        plan.predicted_speedup = 1.0;
        plan.optimized_query = where_clause;
        plan.execution_notes = "No WHERE clause predicates";
        return plan;
    }
    
    // Note: QueryRewriter can be integrated as a future enhancement
    // For now, we use the original query
    plan.optimized_query = where_clause;
    
    // Single predicate optimization
    if (predicates.size() == 1) {
        const auto& pred = predicates[0];
        
        // Create IndexStats for the available index
        std::vector<optimization::IndexAdvisor::IndexStats> stats;
        if (!available_indexes.empty()) {
            optimization::IndexAdvisor::IndexStats stat;
            stat.index_name = available_indexes[0] + "_idx";
            stat.column_name = pred.column;
            stat.cardinality = table_size / 10;  // Estimate cardinality
            stat.avg_selectivity = 0.1;
            stat.lookups_count = 0;
            stat.avg_lookup_time_ms = 0.0;
            stat.is_composite = false;
            stats.push_back(stat);
        }
        
        // Get index advisor recommendation
        auto recommendation = index_advisor_.recommend_index(
            pred.column,
            pred.op,
            table_size,
            stats);
        
        if (recommendation.selected_strategy == "index_btree" || 
            recommendation.selected_strategy == "index_hash") {
            plan.strategy = "index_single";
            plan.indexes_used = {recommendation.primary_index};
            plan.predicted_speedup = recommendation.indexed_scan_cost.estimated_speedup;
            plan.execution_notes = "Index recommended. Estimated " + 
                std::to_string(static_cast<int>(recommendation.indexed_scan_cost.estimated_speedup)) + "x speedup";
        } else {
            plan.strategy = "full_scan";
            plan.predicted_speedup = 1.0;
            plan.execution_notes = "Full scan recommended: low selectivity or high cardinality";
        }
        
        total_queries_optimized_++;
        total_speedup_ += plan.predicted_speedup;
        return plan;
    }
    
    // Multi-predicate optimization
    // Convert ParsedPredicates to PredicateInfo format
    std::vector<optimization::CompositeIndexOptimizer::PredicateInfo> pred_info;
    for (const auto& p : predicates) {
        optimization::CompositeIndexOptimizer::PredicateInfo info;
        info.column = p.column;
        info.operator_type = p.op;
        info.value = p.value;
        info.logical_op = "AND";  // Default to AND
        info.estimated_selectivity = 0.1;  // Default estimate
        pred_info.push_back(info);
    }
    
    auto multi_plan = composite_optimizer_.plan_multi_predicate_query(
        pred_info,
        table_size,
        available_indexes);
    
    plan.strategy = [&multi_plan]() {
        switch (multi_plan.selected_strategy) {
            case optimization::CompositeIndexOptimizer::Strategy::INDEX_INTERSECTION:
                return "index_intersection";
            case optimization::CompositeIndexOptimizer::Strategy::INDEX_UNION:
                return "index_union";
            case optimization::CompositeIndexOptimizer::Strategy::COMPOSITE_INDEX:
                return "index_composite";
            default:
                return "full_scan";
        }
    }();
    
    plan.indexes_used = multi_plan.indexes_used;
    plan.predicted_speedup = multi_plan.estimated_speedup;
    
    std::stringstream notes;
    notes << "Multi-predicate optimization (" << predicates.size() << " predicates). "
          << "Strategy: " << plan.strategy << ". "
          << "Estimated speedup: " << plan.predicted_speedup << "x. "
          << "Execution order: " << multi_plan.execution_order;
    
    plan.execution_notes = notes.str();
    
    total_queries_optimized_++;
    total_speedup_ += plan.predicted_speedup;
    
    return plan;
}

void Phase44QueryOptimizer::register_index(
    const std::string& index_name,
    const std::string& column_name,
    size_t cardinality,
    bool is_composite) {
    
    // Create index stats for tracking
    // This is for future enhancements - both modules track indexes internally
    (void)index_name;      // Suppress unused warning
    (void)column_name;
    (void)cardinality;
    (void)is_composite;
}

void Phase44QueryOptimizer::record_execution_result(
    const std::string& query,
    const std::string& strategy_used,
    size_t rows_examined,
    size_t rows_matched,
    double execution_time_ms) {
    
    // Learn from actual execution for cost model refinement
    if (rows_examined > 0) {
        // Extract column name from query (simplified - assumes format "column op value")
        std::string column_name = "unknown";
        size_t space_pos = query.find(' ');
        if (space_pos != std::string::npos) {
            column_name = query.substr(0, space_pos);
        }
        
        // Update index advisor patterns
        try {
            index_advisor_.learn_from_execution(
                column_name,
                strategy_used,
                rows_examined,
                rows_matched,
                execution_time_ms);
        } catch (...) {
            // Silently ignore learning errors
        }
    }
}

std::string Phase44QueryOptimizer::get_optimization_stats() const {
    std::stringstream stats;
    stats << "=== Phase 4.4 Query Optimization Statistics ===\n";
    stats << "Total queries optimized: " << total_queries_optimized_ << "\n";
    
    if (total_queries_optimized_ > 0) {
        double avg_speedup = total_speedup_ / total_queries_optimized_;
        stats << "Average predicted speedup: " << avg_speedup << "x\n";
    }
    
    stats << "\n=== Index Advisor Statistics ===\n";
    stats << "IndexAdvisor module initialized and ready for cost estimation\n";
    
    return stats.str();
}

std::vector<Phase44QueryOptimizer::ParsedPredicate> Phase44QueryOptimizer::parse_where_clause(
    const std::string& where_clause) {
    
    std::vector<ParsedPredicate> predicates;
    
    // Simple parser for WHERE clauses with AND operators
    // Format: "column1 op value1 AND column2 op value2 ..."
    
    std::string clause = where_clause;
    
    // Remove "WHERE " prefix if present
    if (clause.find("WHERE ") == 0 || clause.find("where ") == 0) {
        clause = clause.substr(6);
    }
    
    // Split by AND
    size_t pos = 0;
    while (pos < clause.length()) {
        // Find the next AND
        size_t and_pos = clause.find(" AND ", pos);
        if (and_pos == std::string::npos) {
            and_pos = clause.find(" and ", pos);
        }
        
        std::string predicate_str = (and_pos == std::string::npos)
            ? clause.substr(pos)
            : clause.substr(pos, and_pos - pos);
        
        // Trim whitespace
        predicate_str.erase(0, predicate_str.find_first_not_of(" \t\n\r"));
        predicate_str.erase(predicate_str.find_last_not_of(" \t\n\r") + 1);
        
        // Parse individual predicate: column op value
        // Operators: =, <, >, <=, >=, !=, LIKE, IN
        
        std::string ops[] = {"<=", ">=", "!=", "=", "<", ">", "LIKE", "IN"};
        
        bool parsed = false;
        for (const auto& op : ops) {
            size_t op_pos = predicate_str.find(op);
            if (op_pos != std::string::npos) {
                ParsedPredicate pred;
                pred.column = predicate_str.substr(0, op_pos);
                pred.op = op;
                pred.value = predicate_str.substr(op_pos + op.length());
                
                // Trim
                pred.column.erase(pred.column.find_last_not_of(" \t\n\r") + 1);
                pred.value.erase(0, pred.value.find_first_not_of(" \t\n\r"));
                pred.value.erase(pred.value.find_last_not_of(" \t\n\r") + 1);
                
                if (!pred.column.empty() && !pred.value.empty()) {
                    predicates.push_back(pred);
                    parsed = true;
                    break;
                }
            }
        }
        
        if (and_pos == std::string::npos) break;
        pos = and_pos + 5;  // Skip " AND "
    }
    
    return predicates;
}

bool Phase44QueryOptimizer::is_composite_predicate(
    const std::vector<ParsedPredicate>& predicates) {
    return predicates.size() > 1;
}

double Phase44QueryOptimizer::estimate_selectivity(const ParsedPredicate& pred) {
    // Estimate selectivity based on operator type
    // These are conservative estimates used when index stats aren't available
    if (pred.op == "=") {
        return 0.01;  // Equality is typically very selective
    } else if (pred.op == "<" || pred.op == ">") {
        return 0.5;   // Inequality typically matches half the rows
    } else if (pred.op == "<=" || pred.op == ">=") {
        return 0.5;
    } else if (pred.op == "!=") {
        return 0.99;  // Not equal matches almost everything
    } else if (pred.op == "LIKE") {
        return 0.1;   // String patterns are moderately selective
    } else if (pred.op == "IN") {
        return 0.05;  // IN clauses are quite selective
    }
    return 0.5;  // Default fallback
}

} // namespace integration
} // namespace lyradb
