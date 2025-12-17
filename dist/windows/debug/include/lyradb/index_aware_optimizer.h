#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>
#include "query_plan.h"

namespace lyradb {

// Forward declaration
class IndexManager;

namespace plan {

// ============================================================================
// Index-Aware Query Optimization
// ============================================================================

/**
 * @brief Enhanced scan node that can use indexes
 */
class IndexedScanNode : public PlanNode {
public:
    IndexedScanNode(const std::string& table_name, long long row_count, long long column_count)
        : table_name_(table_name), row_count_(row_count), column_count_(column_count) {}

    NodeType type() const override { return NodeType::TableScan; }
    std::string to_string() const override;
    long long estimated_rows() const override { return estimated_rows_; }
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override { return {}; }
    const std::vector<PlanNode*> children() const override { return {}; }

    const std::string& table_name() const { return table_name_; }
    long long row_count() const { return row_count_; }
    long long column_count() const { return column_count_; }

    // Index support
    void use_index(const std::string& index_name, const std::string& column,
                   const std::string& index_type) {
        index_name_ = index_name;
        index_column_ = column;
        index_type_ = index_type;
        uses_index_ = true;
    }

    bool uses_index() const { return uses_index_; }
    const std::string& index_name() const { return index_name_; }
    const std::string& index_column() const { return index_column_; }
    const std::string& index_type() const { return index_type_; }
    
    void set_estimated_rows(long long rows) { estimated_rows_ = rows; }

private:
    std::string table_name_;
    long long row_count_;
    long long column_count_;
    long long estimated_rows_;
    bool uses_index_ = false;
    std::string index_name_;
    std::string index_column_;
    std::string index_type_;
};

/**
 * @brief Specialized filter node for indexed access
 */
class IndexedFilterNode : public PlanNode {
public:
    enum class PredicateType {
        EQUALITY,       // col = value
        RANGE,          // col > value, col < value
        IN_LIST,        // col IN (v1, v2, ...)
        BETWEEN,        // col BETWEEN v1 AND v2
        NOT_EQUAL       // col != value
    };

    IndexedFilterNode(std::unique_ptr<PlanNode> child, const std::string& condition)
        : child_(std::move(child)), condition_(condition) {}

    NodeType type() const override { return NodeType::Filter; }
    std::string to_string() const override;
    long long estimated_rows() const override;
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override;
    const std::vector<PlanNode*> children() const override;

    PlanNode* child() { return child_.get(); }
    const PlanNode* child() const { return child_.get(); }
    const std::string& condition() const { return condition_; }
    void set_selectivity(double sel) { selectivity_ = sel; }
    double selectivity() const { return selectivity_; }

    // Index-specific information
    void set_predicate_info(PredicateType type, const std::string& column,
                           double selectivity) {
        predicate_type_ = type;
        predicate_column_ = column;
        selectivity_ = selectivity;
    }

    PredicateType predicate_type() const { return predicate_type_; }
    const std::string& predicate_column() const { return predicate_column_; }

private:
    std::unique_ptr<PlanNode> child_;
    std::string condition_;
    double selectivity_ = 0.5;
    PredicateType predicate_type_ = PredicateType::EQUALITY;
    std::string predicate_column_;
};

/**
 * @brief Statistics for index selection
 */
struct IndexSelectionStats {
    std::string table_name;
    std::string column_name;
    long long row_count = 0;
    long long cardinality = 0;  // distinct values
    double selectivity = 0.5;
    std::string predicate_type;
    
    // Estimated cost comparison
    double full_scan_cost = 0.0;
    double index_scan_cost = 0.0;
    
    bool should_use_index() const {
        return index_scan_cost < full_scan_cost;
    }
};

/**
 * @brief Index-aware query optimizer
 * 
 * Extends the base optimizer with:
 * - Index opportunity detection
 * - Index selection based on predicate and cardinality
 * - Cost-based index selection
 * - Bitmap index optimization for multiple filters
 */
class IndexAwareOptimizer {
public:
    explicit IndexAwareOptimizer(IndexManager* index_manager)
        : index_manager_(index_manager) {}

    // Main optimization entry point
    std::unique_ptr<QueryPlan> optimize_with_indexes(const QueryPlan& plan);

    // Index detection and selection
    std::vector<IndexSelectionStats> analyze_index_opportunities(const QueryPlan& plan);
    
    // Apply index selection to plan
    std::unique_ptr<QueryPlan> apply_index_selection(const QueryPlan& plan,
                                                     const IndexSelectionStats& stats);

    // Predicate analysis
    IndexedFilterNode::PredicateType analyze_predicate(const std::string& condition,
                                                       std::string& column_name);

    // Cost estimation with indexes
    double estimate_scan_cost(const IndexSelectionStats& stats);
    double estimate_index_scan_cost(const IndexSelectionStats& stats,
                                   const std::string& index_type);

private:
    IndexManager* index_manager_;

    // Helper methods
    PlanNode* optimize_node(PlanNode* node);
    void detect_index_opportunities(PlanNode* node,
                                   std::vector<IndexSelectionStats>& opportunities);
    
    // Predicate parsing helpers
    bool extract_column_and_value(const std::string& condition,
                                 std::string& column,
                                 std::string& value);
    bool is_equality_predicate(const std::string& condition);
    bool is_range_predicate(const std::string& condition);
    bool is_in_list_predicate(const std::string& condition);

    // Cost model constants
    static constexpr double BTREE_ACCESS_COST = 0.8;      // log(N) is cheap
    static constexpr double HASH_ACCESS_COST = 1.0;       // O(1) but slightly more overhead
    static constexpr double BITMAP_ACCESS_COST = 0.5;     // Very fast for cardinality < 100
    static constexpr double FULL_SCAN_COST = 1.0;         // Baseline
};

} // namespace plan
} // namespace lyradb
