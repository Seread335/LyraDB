/**
 * @file range_query_optimizer.cpp
 * @brief Implementation of range predicate detection and optimization
 * 
 * Phase 4.2: B-Tree Range Query Support
 */

#include "lyradb/range_query_optimizer.h"
#include "lyradb/sql_parser.h"
#include <algorithm>
#include <sstream>

namespace lyradb {
namespace query {

// ============================================================================
// RangeBound Implementation
// ============================================================================

std::string RangeBound::to_string() const {
    std::string result = column_name;
    switch (type) {
        case Type::GREATER_THAN:
            result += " > ";
            break;
        case Type::GREATER_EQUAL:
            result += " >= ";
            break;
        case Type::LESS_THAN:
            result += " < ";
            break;
        case Type::LESS_EQUAL:
            result += " <= ";
            break;
        case Type::EQUAL:
            result += " = ";
            break;
        case Type::BETWEEN:
            result += " BETWEEN ";
            break;
    }
    result += value;
    return result;
}

// ============================================================================
// RangePredicate Implementation
// ============================================================================

bool RangePredicate::is_sargable() const {
    // A range predicate is searchable-argument (SARGABLE) if:
    // 1. It has at least one range bound
    // 2. All bounds refer to indexed columns
    return !bounds.empty() && !columns.empty();
}

std::string RangePredicate::to_string() const {
    std::string result = "RangePredicate[table=" + table_name;
    result += ", columns=(";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) result += ",";
        result += columns[i];
    }
    result += "), bounds=(";
    for (size_t i = 0; i < bounds.size(); ++i) {
        if (i > 0) result += " AND ";
        result += bounds[i].to_string();
    }
    result += ")]";
    return result;
}

// ============================================================================
// IndexRecommendation Implementation
// ============================================================================

std::string IndexRecommendation::to_string() const {
    std::stringstream ss;
    ss << "IndexRec[index=" << index_name;
    ss << ", type=" << index_type;
    ss << ", columns=(";
    for (size_t i = 0; i < columns.size(); ++i) {
        if (i > 0) ss << ",";
        ss << columns[i];
    }
    ss << "), selectivity=" << selectivity;
    ss << ", applicable=" << (can_use_index ? "yes" : "no") << "]";
    return ss.str();
}

// ============================================================================
// RangeQueryOptimizer Implementation
// ============================================================================

RangeQueryOptimizer::RangeQueryOptimizer() {
}

RangeQueryOptimizer::~RangeQueryOptimizer() {
}

bool RangeQueryOptimizer::is_range_operator(const std::string& op_name) {
    return op_name == ">" || op_name == "<" || 
           op_name == ">=" || op_name == "<=" || 
           op_name == "BETWEEN";
}

std::string RangeQueryOptimizer::extract_simple_column_name(const Expression* expr) {
    if (!expr) return "";
    
    // Check if this is a simple column reference (not a function call)
    // This would require dynamic_cast to ColumnRefExpr
    // For now, return empty as we need the expression class definition
    
    return "";
}

std::string RangeQueryOptimizer::extract_literal_value(const Expression* expr) {
    if (!expr) return "";
    
    // Check if this is a literal value
    // This would require dynamic_cast to LiteralExpr
    // For now, return empty as we need the expression class definition
    
    return "";
}

std::vector<RangePredicate> RangeQueryOptimizer::extract_range_predicates(
    const Expression* where_clause,
    const std::string& table_name) {
    
    std::vector<RangePredicate> predicates;
    
    if (!where_clause) {
        return predicates;
    }
    
    traverse_and_collect_predicates(where_clause, table_name, predicates);
    
    return predicates;
}

void RangeQueryOptimizer::traverse_and_collect_predicates(
    const Expression* expr,
    const std::string& table_name,
    std::vector<RangePredicate>& predicates) {
    
    if (!expr) return;
    
    // Try to detect this expression as a range bound
    auto bound = detect_range_bound(expr);
    if (bound) {
        RangePredicate pred;
        pred.table_name = table_name;
        pred.columns.push_back(bound->column_name);
        pred.bounds.push_back(*bound);
        predicates.push_back(pred);
    }
    
    // For AND expressions, recursively process both sides
    // For OR expressions, we don't optimize (complex case)
    // This requires accessing BinaryExpr properties
}

std::optional<RangeBound> RangeQueryOptimizer::detect_range_bound(const Expression* expr) {
    if (!expr) {
        return std::nullopt;
    }
    
    // This requires analyzing the expression tree structure
    // Would need to dynamic_cast to BinaryExpr and check operator type
    // Implementation would follow this pattern:
    // if (auto binary = dynamic_cast<const BinaryExpr*>(expr)) {
    //     if (is_range_operator(binary->op_)) {
    //         auto col = extract_simple_column_name(binary->left_);
    //         auto val = extract_literal_value(binary->right_);
    //         if (!col.empty() && !val.empty()) {
    //             RangeBound bound;
    //             bound.column_name = col;
    //             bound.value = val;
    //             // Determine type based on operator
    //             return bound;
    //         }
    //     }
    // }
    
    return std::nullopt;
}

RangePredicate RangeQueryOptimizer::merge_bounds(const std::vector<RangeBound>& bounds) {
    RangePredicate result;
    result.bounds = bounds;
    
    // Consolidate columns from all bounds
    for (const auto& bound : bounds) {
        if (std::find(result.columns.begin(), result.columns.end(), 
                      bound.column_name) == result.columns.end()) {
            result.columns.push_back(bound.column_name);
        }
    }
    
    return result;
}

// ============================================================================
// BTreeIndexSelector Implementation
// ============================================================================

BTreeIndexSelector::BTreeIndexSelector() {
}

BTreeIndexSelector::~BTreeIndexSelector() {
}

std::optional<IndexRecommendation> BTreeIndexSelector::select_index(
    const RangePredicate& predicate,
    const std::vector<std::string>& available_indexes,
    const std::string& table_name) {
    
    if (!predicate.is_sargable() || available_indexes.empty()) {
        return std::nullopt;
    }
    
    // Find best matching index
    IndexRecommendation best;
    best.can_use_index = false;
    
    for (const auto& index_name : available_indexes) {
        // Check if this is a B-tree index (has _btree suffix)
        if (index_name.find("_btree") == std::string::npos) {
            continue;  // Skip non-B-tree indexes
        }
        
        // Check if index covers the predicate columns
        if (is_applicable_index(predicate.columns, predicate.columns)) {
            IndexRecommendation rec;
            rec.index_name = index_name;
            rec.index_type = "BTREE";
            rec.columns = predicate.columns;
            rec.selectivity = estimate_selectivity(predicate);
            rec.can_use_index = true;
            
            // Use first matching B-tree index
            return rec;
        }
    }
    
    return std::nullopt;
}

float BTreeIndexSelector::estimate_selectivity(const RangePredicate& predicate) {
    // Simple selectivity estimation based on predicate type
    // Real implementation would use table statistics
    
    if (predicate.bounds.empty()) {
        return 1.0f;  // No bounds = full table scan
    }
    
    // Assume different selectivity based on bound type
    float selectivity = 1.0f;
    for (const auto& bound : predicate.bounds) {
        switch (bound.type) {
            case RangeBound::Type::EQUAL:
                selectivity *= 0.001f;  // ~0.1% selectivity
                break;
            case RangeBound::Type::BETWEEN:
                selectivity *= 0.1f;    // ~10% selectivity
                break;
            case RangeBound::Type::GREATER_THAN:
            case RangeBound::Type::LESS_THAN:
                selectivity *= 0.5f;    // ~50% selectivity
                break;
            case RangeBound::Type::GREATER_EQUAL:
            case RangeBound::Type::LESS_EQUAL:
                selectivity *= 0.5f;    // ~50% selectivity
                break;
        }
    }
    
    return std::max(0.0f, std::min(1.0f, selectivity));
}

bool BTreeIndexSelector::is_applicable_index(
    const std::vector<std::string>& index_columns,
    const std::vector<std::string>& predicate_columns) const {
    
    // Index is applicable if:
    // 1. Single-column index matches predicate column
    // 2. Multi-column index has predicate columns as prefix
    
    if (index_columns.empty() || predicate_columns.empty()) {
        return false;
    }
    
    // Check if all predicate columns are in index (in order or as subset)
    for (size_t i = 0; i < predicate_columns.size(); ++i) {
        if (i >= index_columns.size()) {
            return false;  // Index has fewer columns than predicate
        }
        
        if (index_columns[i] != predicate_columns[i]) {
            return false;  // Column mismatch
        }
    }
    
    return true;
}

} // namespace query
} // namespace lyradb
