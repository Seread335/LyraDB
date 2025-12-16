/**
 * @file range_query_optimizer.h
 * @brief Range predicate detection and optimization for B-tree indexes
 * 
 * Phase 4.2: B-Tree Range Query Support
 * Detects range predicates (>, <, >=, <=, BETWEEN) and routes to B-tree indexes
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <variant>

namespace lyradb {
namespace query {

// Forward declarations
class Expression;
class BinaryExpr;
class SelectStatement;

/**
 * @brief Range bound specification
 * Represents a single bound in a range predicate (e.g., x > 5 or y <= 100)
 */
struct RangeBound {
    enum class Type {
        GREATER_THAN,        // >
        GREATER_EQUAL,       // >=
        LESS_THAN,          // <
        LESS_EQUAL,         // <=
        EQUAL,              // =
        BETWEEN             // BETWEEN x AND y
    };
    
    Type type;
    std::string column_name;
    std::string value;  // String representation of the bound
    
    std::string to_string() const;
    bool is_range_predicate() const {
        return type != Type::EQUAL;
    }
};

/**
 * @brief Range predicate group
 * Groups related range predicates on the same column(s)
 */
struct RangePredicate {
    std::string table_name;
    std::vector<std::string> columns;  // For composite predicates
    std::vector<RangeBound> bounds;
    bool is_sargable() const;  // Searchable using index
    std::string to_string() const;
};

/**
 * @brief Index recommendation for range queries
 */
struct IndexRecommendation {
    std::string index_name;
    std::string index_type;  // "HASH" or "BTREE"
    std::vector<std::string> columns;
    float selectivity;       // Estimated fraction of rows matching predicate
    bool can_use_index;
    
    std::string to_string() const;
};

/**
 * @brief Range Query Optimizer
 * Detects and optimizes range predicates in WHERE clauses
 */
class RangeQueryOptimizer {
public:
    RangeQueryOptimizer();
    ~RangeQueryOptimizer();
    
    /**
     * @brief Extract range predicates from WHERE clause
     * @param where_clause Root of expression tree
     * @param table_name Name of table being queried
     * @return Vector of detected range predicates
     */
    std::vector<RangePredicate> extract_range_predicates(
        const Expression* where_clause,
        const std::string& table_name
    );
    
    /**
     * @brief Detect individual range bounds in expressions
     * @param expr Expression to analyze
     * @return Optional RangeBound if expression is a range predicate
     */
    std::optional<RangeBound> detect_range_bound(const Expression* expr);
    
    /**
     * @brief Check if expression uses a range operator (>, <, >=, <=)
     * @param op_name Operator name
     * @return True if operator is a range operator
     */
    static bool is_range_operator(const std::string& op_name);
    
    /**
     * @brief Check if column reference is simple (no functions)
     * @param expr Expression to check
     * @return Column name if simple, empty string otherwise
     */
    static std::string extract_simple_column_name(const Expression* expr);
    
    /**
     * @brief Extract literal value from expression
     * @param expr Expression to check
     * @return String representation of literal value
     */
    static std::string extract_literal_value(const Expression* expr);
    
private:
    /**
     * @brief Recursively traverse expression tree to find range predicates
     * @param expr Current expression node
     * @param table_name Table name
     * @param predicates Output vector of found predicates
     */
    void traverse_and_collect_predicates(
        const Expression* expr,
        const std::string& table_name,
        std::vector<RangePredicate>& predicates
    );
    
    /**
     * @brief Merge related range bounds on same column
     * @param bounds Vector of bounds to merge
     * @return Merged predicate with optimized bounds
     */
    RangePredicate merge_bounds(const std::vector<RangeBound>& bounds);
};

/**
 * @brief B-Tree Index Selector
 * Recommends appropriate indexes for range queries
 */
class BTreeIndexSelector {
public:
    BTreeIndexSelector();
    ~BTreeIndexSelector();
    
    /**
     * @brief Find best B-tree index for range predicate
     * @param predicate Range predicate to optimize
     * @param available_indexes List of available indexes
     * @param table_name Name of table
     * @return Recommended index, or empty if none suitable
     */
    std::optional<IndexRecommendation> select_index(
        const RangePredicate& predicate,
        const std::vector<std::string>& available_indexes,
        const std::string& table_name
    );
    
    /**
     * @brief Estimate selectivity of range predicate
     * @param predicate Range predicate
     * @return Estimated selectivity (0.0 to 1.0)
     */
    float estimate_selectivity(const RangePredicate& predicate);
    
    /**
     * @brief Check if index can be used for predicate
     * @param index_columns Columns in index
     * @param predicate_columns Columns in predicate
     * @return True if index is applicable
     */
    bool is_applicable_index(
        const std::vector<std::string>& index_columns,
        const std::vector<std::string>& predicate_columns
    ) const;
};

} // namespace query
} // namespace lyradb
