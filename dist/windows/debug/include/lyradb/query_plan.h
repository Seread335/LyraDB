#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <unordered_set>
#include <unordered_map>

namespace lyradb {

// Forward declarations
class SelectStatement;
class Expression;

namespace plan {

// ============================================================================
// Plan Node Type Enumeration
// ============================================================================

enum class NodeType {
    TableScan,
    Filter,
    Project,
    Aggregate,
    Join,
    Sort,
    Limit,
};

enum class JoinAlgorithm {
    HASH_JOIN,      // Hash join (best for large tables)
    NESTED_LOOP,    // Nested loop join (small tables)
    SORT_MERGE      // Sort-merge join (pre-sorted data)
};

// ============================================================================
// Plan Node Base Class
// ============================================================================

/**
 * @brief Base class for query plan nodes
 */
class PlanNode {
public:
    virtual ~PlanNode() = default;
    virtual NodeType type() const = 0;
    virtual std::string to_string() const = 0;
    virtual long long estimated_rows() const = 0;
    virtual long long estimated_memory() const = 0;
    virtual std::vector<PlanNode*> children() = 0;
    virtual const std::vector<PlanNode*> children() const = 0;
};

// ============================================================================
// Specific Plan Node Types
// ============================================================================

class ScanNode : public PlanNode {
public:
    ScanNode(const std::string& table_name, long long row_count, long long column_count)
        : table_name_(table_name), row_count_(row_count), column_count_(column_count) {}

    NodeType type() const override { return NodeType::TableScan; }
    std::string to_string() const override;
    long long estimated_rows() const override { return row_count_; }
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override { return {}; }
    const std::vector<PlanNode*> children() const override { return {}; }

    const std::string& table_name() const { return table_name_; }
    long long row_count() const { return row_count_; }
    long long column_count() const { return column_count_; }

private:
    std::string table_name_;
    long long row_count_;
    long long column_count_;
};

class FilterNode : public PlanNode {
public:
    FilterNode(std::unique_ptr<PlanNode> child, const std::string& condition)
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

private:
    std::unique_ptr<PlanNode> child_;
    std::string condition_;
    double selectivity_ = 0.5;
};

class ProjectNode : public PlanNode {
public:
    ProjectNode(std::unique_ptr<PlanNode> child, const std::vector<std::string>& columns)
        : child_(std::move(child)), columns_(columns) {}

    NodeType type() const override { return NodeType::Project; }
    std::string to_string() const override;
    long long estimated_rows() const override;
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override;
    const std::vector<PlanNode*> children() const override;

    PlanNode* child() { return child_.get(); }
    const PlanNode* child() const { return child_.get(); }
    const std::vector<std::string>& columns() const { return columns_; }

private:
    std::unique_ptr<PlanNode> child_;
    std::vector<std::string> columns_;
};

class JoinNode : public PlanNode {
public:
    JoinNode(std::unique_ptr<PlanNode> left, std::unique_ptr<PlanNode> right,
             const std::string& condition, JoinAlgorithm algorithm = JoinAlgorithm::HASH_JOIN)
        : left_(std::move(left)), right_(std::move(right)), 
          condition_(condition), algorithm_(algorithm) {}

    NodeType type() const override { return NodeType::Join; }
    std::string to_string() const override;
    long long estimated_rows() const override;
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override;
    const std::vector<PlanNode*> children() const override;

    PlanNode* left() { return left_.get(); }
    const PlanNode* left() const { return left_.get(); }
    PlanNode* right() { return right_.get(); }
    const PlanNode* right() const { return right_.get(); }

    const std::string& condition() const { return condition_; }
    JoinAlgorithm algorithm() const { return algorithm_; }
    void set_algorithm(JoinAlgorithm algo) { algorithm_ = algo; }

private:
    std::unique_ptr<PlanNode> left_;
    std::unique_ptr<PlanNode> right_;
    std::string condition_;
    JoinAlgorithm algorithm_;
};

class AggregateNode : public PlanNode {
public:
    AggregateNode(std::unique_ptr<PlanNode> child,
                  const std::vector<std::string>& group_by_cols,
                  const std::vector<std::string>& aggregate_exprs)
        : child_(std::move(child)), group_by_cols_(group_by_cols),
          aggregate_exprs_(aggregate_exprs) {}

    NodeType type() const override { return NodeType::Aggregate; }
    std::string to_string() const override;
    long long estimated_rows() const override;
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override;
    const std::vector<PlanNode*> children() const override;

    PlanNode* child() { return child_.get(); }
    const PlanNode* child() const { return child_.get(); }
    const std::vector<std::string>& group_by_cols() const { return group_by_cols_; }
    const std::vector<std::string>& aggregate_exprs() const { return aggregate_exprs_; }
    void set_cardinality(long long card) { cardinality_ = card; }
    long long cardinality() const { return cardinality_; }

private:
    std::unique_ptr<PlanNode> child_;
    std::vector<std::string> group_by_cols_;
    std::vector<std::string> aggregate_exprs_;
    long long cardinality_ = -1;
};

class SortNode : public PlanNode {
public:
    struct SortKey {
        std::string column;
        bool ascending;
    };

    SortNode(std::unique_ptr<PlanNode> child, const std::vector<SortKey>& sort_keys)
        : child_(std::move(child)), sort_keys_(sort_keys) {}

    NodeType type() const override { return NodeType::Sort; }
    std::string to_string() const override;
    long long estimated_rows() const override;
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override;
    const std::vector<PlanNode*> children() const override;

    PlanNode* child() { return child_.get(); }
    const PlanNode* child() const { return child_.get(); }
    const std::vector<SortKey>& sort_keys() const { return sort_keys_; }

private:
    std::unique_ptr<PlanNode> child_;
    std::vector<SortKey> sort_keys_;
};

class LimitNode : public PlanNode {
public:
    LimitNode(std::unique_ptr<PlanNode> child, long long limit, long long offset = 0)
        : child_(std::move(child)), limit_(limit), offset_(offset) {}

    NodeType type() const override { return NodeType::Limit; }
    std::string to_string() const override;
    long long estimated_rows() const override;
    long long estimated_memory() const override;
    std::vector<PlanNode*> children() override;
    const std::vector<PlanNode*> children() const override;

    PlanNode* child() { return child_.get(); }
    const PlanNode* child() const { return child_.get(); }
    long long limit() const { return limit_; }
    long long offset() const { return offset_; }

private:
    std::unique_ptr<PlanNode> child_;
    long long limit_;
    long long offset_;
};

// ============================================================================
// Query Plan Container
// ============================================================================

class QueryPlan {
public:
    explicit QueryPlan(std::unique_ptr<PlanNode> root) : root_(std::move(root)) {}

    PlanNode* root() { return root_.get(); }
    const PlanNode* root() const { return root_.get(); }

    std::string to_string() const;
    long long estimated_rows() const;
    long long estimated_memory() const;
    double estimated_cost() const;

    struct OptimizationStats {
        int filter_pushdown_count = 0;
        int column_prune_count = 0;
        int join_reorder_count = 0;
        int redundant_sort_removed = 0;
        std::string optimization_notes;
    };

    const OptimizationStats& stats() const { return stats_; }
    OptimizationStats& stats() { return stats_; }

private:
    std::unique_ptr<PlanNode> root_;
    OptimizationStats stats_;
};

// ============================================================================
// Query Optimizer
// ============================================================================

class QueryOptimizer {
public:
    QueryOptimizer() = default;

    // Convert SelectStatement to optimized QueryPlan
    std::unique_ptr<QueryPlan> optimize(const SelectStatement& stmt);

    // Optimize existing plan
    std::unique_ptr<QueryPlan> optimize_plan(const QueryPlan& plan);

private:
    // Optimization passes
    std::unique_ptr<QueryPlan> apply_predicate_pushdown(const QueryPlan& plan);
    std::unique_ptr<QueryPlan> apply_column_pruning(const QueryPlan& plan);
    std::unique_ptr<QueryPlan> apply_join_reordering(const QueryPlan& plan);
    std::unique_ptr<QueryPlan> remove_redundant_sorts(const QueryPlan& plan);

    // Helper methods
    std::unique_ptr<PlanNode> build_plan_tree(const SelectStatement& stmt);
};

} // namespace plan
} // namespace lyradb
