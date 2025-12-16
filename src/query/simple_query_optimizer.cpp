#include "lyradb/simple_query_optimizer.h"
#include <algorithm>
#include <sstream>

namespace lyradb {
namespace integration {

SimpleQueryOptimizer::SimpleQueryOptimizer()
    : total_queries_optimized_(0), total_predicted_speedup_(0.0) {
}

SimpleQueryOptimizer::~SimpleQueryOptimizer() {
}

SimpleQueryOptimizer::Plan SimpleQueryOptimizer::optimize(
    const std::string& where_clause,
    size_t table_size,
    const std::vector<std::string>& available_indexes) {
    
    Plan plan;
    plan.strategy = Strategy::FULL_SCAN;
    plan.predicted_speedup = 1.0;
    plan.explanation = "Default: full scan";
    
    // Empty WHERE clause - full scan
    if (where_clause.empty()) {
        return plan;
    }
    
    // Count predicates
    size_t pred_count = count_predicates(where_clause);
    
    // Single predicate optimization
    if (pred_count == 1) {
        std::string column, op, value;
        
        // Try equality first
        if (parse_equality_predicate(where_clause, column, value)) {
            // Check if we have an index on this column
            auto it = indexes_.find(column);
            if (it != indexes_.end() && !available_indexes.empty()) {
                plan.strategy = Strategy::INDEX_SINGLE;
                plan.predicted_speedup = 100.0;  // Equality is very fast
                plan.indexes_to_use = {it->second.name};
                plan.explanation = "B-tree index on " + column + " (equality lookup)";
            } else {
                plan.explanation = "No index on " + column + " - full scan";
            }
            total_queries_optimized_++;
            total_predicted_speedup_ += plan.predicted_speedup;
            return plan;
        }
        
        // Try range predicate
        if (parse_range_predicate(where_clause, column, op, value)) {
            auto it = indexes_.find(column);
            if (it != indexes_.end() && !available_indexes.empty()) {
                plan.strategy = Strategy::INDEX_RANGE;
                plan.predicted_speedup = 50.0;  // Range scans are good
                plan.indexes_to_use = {it->second.name};
                plan.explanation = "B-tree range scan on " + column + " (" + op + ")";
            } else {
                plan.explanation = "No index on " + column + " - full scan";
            }
            total_queries_optimized_++;
            total_predicted_speedup_ += plan.predicted_speedup;
            return plan;
        }
    }
    
    // Multi-predicate optimization
    if (pred_count > 1) {
        // Check for AND vs OR
        if (where_clause.find(" AND ") != std::string::npos) {
            plan.strategy = Strategy::INDEX_INTERSECTION;
            plan.predicted_speedup = 20.0;  // Multiple indexes combined
            plan.explanation = "Index intersection for AND predicates";
        } else if (where_clause.find(" OR ") != std::string::npos) {
            plan.strategy = Strategy::INDEX_UNION;
            plan.predicted_speedup = 10.0;  // Union is less efficient
            plan.explanation = "Index union for OR predicates";
        }
        total_queries_optimized_++;
        total_predicted_speedup_ += plan.predicted_speedup;
        return plan;
    }
    
    // Default: full scan
    total_queries_optimized_++;
    total_predicted_speedup_ += plan.predicted_speedup;
    return plan;
}

void SimpleQueryOptimizer::register_index(
    const std::string& index_name,
    const std::string& column_name) {
    IndexInfo info;
    info.name = index_name;
    info.column = column_name;
    indexes_[column_name] = info;
}

void SimpleQueryOptimizer::record_result(
    const std::string& where_clause,
    Strategy strategy_used,
    size_t rows_examined,
    size_t rows_matched,
    double execution_time_ms) {
    
    // Future: use actual results to refine cost model
    (void)where_clause;
    (void)strategy_used;
    (void)rows_examined;
    (void)rows_matched;
    (void)execution_time_ms;
}

bool SimpleQueryOptimizer::parse_equality_predicate(
    const std::string& where_clause,
    std::string& column,
    std::string& value) {
    
    // Look for "column = value" pattern
    size_t eq_pos = where_clause.find(" = ");
    if (eq_pos == std::string::npos) {
        eq_pos = where_clause.find("=");
        if (eq_pos == std::string::npos || eq_pos == 0) {
            return false;
        }
    }
    
    column = where_clause.substr(0, eq_pos);
    // Trim whitespace
    column.erase(column.find_last_not_of(" \t") + 1);
    
    value = where_clause.substr(eq_pos + (where_clause[eq_pos] == ' ' ? 3 : 1));
    value.erase(0, value.find_first_not_of(" \t"));
    
    return !column.empty() && !value.empty();
}

bool SimpleQueryOptimizer::parse_range_predicate(
    const std::string& where_clause,
    std::string& column,
    std::string& op,
    std::string& value) {
    
    // Look for range operators: >, <, >=, <=
    const char* ops[] = {"<=", ">=", "<", ">"};
    
    for (const char* op_str : ops) {
        size_t op_pos = where_clause.find(op_str);
        if (op_pos != std::string::npos && op_pos > 0) {
            column = where_clause.substr(0, op_pos);
            column.erase(column.find_last_not_of(" \t") + 1);
            
            op = op_str;
            
            value = where_clause.substr(op_pos + strlen(op_str));
            value.erase(0, value.find_first_not_of(" \t"));
            
            return !column.empty() && !value.empty();
        }
    }
    
    return false;
}

size_t SimpleQueryOptimizer::count_predicates(const std::string& where_clause) {
    if (where_clause.empty()) {
        return 0;
    }
    
    size_t count = 1;
    size_t and_count = 0, or_count = 0;
    
    // Count AND operators
    size_t pos = 0;
    while ((pos = where_clause.find(" AND ", pos)) != std::string::npos) {
        and_count++;
        pos += 5;
    }
    
    // Count OR operators
    pos = 0;
    while ((pos = where_clause.find(" OR ", pos)) != std::string::npos) {
        or_count++;
        pos += 4;
    }
    
    return count + and_count + or_count;
}

} // namespace integration
} // namespace lyradb
