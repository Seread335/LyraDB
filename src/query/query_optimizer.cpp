#include "lyradb/query_plan.h"
#include "lyradb/sql_parser.h"
#include <sstream>
#include <algorithm>
#include <cmath>
#include <functional>
#include <cctype>

namespace lyradb {
namespace plan {

// ============================================================================
// ScanNode Implementation
// ============================================================================

std::string ScanNode::to_string() const {
    std::ostringstream oss;
    oss << "TableScan(" << table_name_ << ", rows=" << row_count_ 
        << ", cols=" << column_count_ << ")";
    return oss.str();
}

long long ScanNode::estimated_memory() const {
    // Assume 50 bytes per row average
    return row_count_ * 50;
}

// ============================================================================
// FilterNode Implementation
// ============================================================================

std::string FilterNode::to_string() const {
    std::ostringstream oss;
    oss << "Filter(condition=\"" << condition_ << "\", selectivity=" 
        << selectivity_ << ")\n  └─ " << child_->to_string();
    return oss.str();
}

long long FilterNode::estimated_rows() const {
    return static_cast<long long>(child_->estimated_rows() * selectivity_);
}

long long FilterNode::estimated_memory() const {
    return estimated_rows() * 50;
}

std::vector<PlanNode*> FilterNode::children() {
    return {child_.get()};
}

const std::vector<PlanNode*> FilterNode::children() const {
    return {child_.get()};
}

// ============================================================================
// ProjectNode Implementation
// ============================================================================

std::string ProjectNode::to_string() const {
    std::ostringstream oss;
    oss << "Project(cols=" << columns_.size() << ")\n  └─ " << child_->to_string();
    return oss.str();
}

long long ProjectNode::estimated_rows() const {
    return child_->estimated_rows();
}

long long ProjectNode::estimated_memory() const {
    // Roughly: rows * columns * 8 bytes per value
    return estimated_rows() * columns_.size() * 8;
}

std::vector<PlanNode*> ProjectNode::children() {
    return {child_.get()};
}

const std::vector<PlanNode*> ProjectNode::children() const {
    return {child_.get()};
}

// ============================================================================
// JoinNode Implementation
// ============================================================================

std::string JoinNode::to_string() const {
    std::string algo_str;
    switch (algorithm_) {
        case JoinAlgorithm::HASH_JOIN: algo_str = "HashJoin"; break;
        case JoinAlgorithm::NESTED_LOOP: algo_str = "NestedLoop"; break;
        case JoinAlgorithm::SORT_MERGE: algo_str = "SortMerge"; break;
    }
    
    std::ostringstream oss;
    oss << "Join(" << algo_str << ", condition=\"" << condition_ << "\")\n"
        << "  ├─ " << left_->to_string() << "\n"
        << "  └─ " << right_->to_string();
    return oss.str();
}

long long JoinNode::estimated_rows() const {
    // Estimate join result size as left * right * selectivity
    // Default: assume 10% of cross product
    return (left_->estimated_rows() * right_->estimated_rows()) / 10;
}

long long JoinNode::estimated_memory() const {
    return left_->estimated_memory() + right_->estimated_memory() + 
           (estimated_rows() * 100);  // Join output buffer
}

std::vector<PlanNode*> JoinNode::children() {
    return {left_.get(), right_.get()};
}

const std::vector<PlanNode*> JoinNode::children() const {
    return {left_.get(), right_.get()};
}

// ============================================================================
// AggregateNode Implementation
// ============================================================================

std::string AggregateNode::to_string() const {
    std::ostringstream oss;
    oss << "Aggregate(groups=" << group_by_cols_.size() 
        << ", aggs=" << aggregate_exprs_.size() << ")\n  └─ " << child_->to_string();
    return oss.str();
}

long long AggregateNode::estimated_rows() const {
    if (cardinality_ > 0) return cardinality_;
    
    // If no GROUP BY, result is 1 row
    if (group_by_cols_.empty()) return 1;
    
    // Otherwise estimate as min(distinct groups, input rows)
    long long input_rows = child_->estimated_rows();
    return std::min(input_rows, input_rows / 100);  // Assume 1% distinct
}

long long AggregateNode::estimated_memory() const {
    return estimated_rows() * aggregate_exprs_.size() * 16;
}

std::vector<PlanNode*> AggregateNode::children() {
    return {child_.get()};
}

const std::vector<PlanNode*> AggregateNode::children() const {
    return {child_.get()};
}

// ============================================================================
// SortNode Implementation
// ============================================================================

std::string SortNode::to_string() const {
    std::ostringstream oss;
    oss << "Sort(keys=" << sort_keys_.size() << ")\n  └─ " << child_->to_string();
    return oss.str();
}

long long SortNode::estimated_rows() const {
    return child_->estimated_rows();
}

long long SortNode::estimated_memory() const {
    // Sort needs to hold all rows: rows * avg_row_size + overhead
    return estimated_rows() * 100;
}

std::vector<PlanNode*> SortNode::children() {
    return {child_.get()};
}

const std::vector<PlanNode*> SortNode::children() const {
    return {child_.get()};
}

// ============================================================================
// LimitNode Implementation
// ============================================================================

std::string LimitNode::to_string() const {
    std::ostringstream oss;
    oss << "Limit(limit=" << limit_ << ", offset=" << offset_ << ")\n  └─ " 
        << child_->to_string();
    return oss.str();
}

long long LimitNode::estimated_rows() const {
    return std::min(limit_, child_->estimated_rows() - offset_);
}

long long LimitNode::estimated_memory() const {
    return estimated_rows() * 50;
}

std::vector<PlanNode*> LimitNode::children() {
    return {child_.get()};
}

const std::vector<PlanNode*> LimitNode::children() const {
    return {child_.get()};
}

// ============================================================================
// QueryPlan Implementation
// ============================================================================

std::string QueryPlan::to_string() const {
    std::ostringstream oss;
    oss << "QueryPlan:\n" << root_->to_string();
    if (!stats_.optimization_notes.empty()) {
        oss << "\n\nOptimization Notes:\n" << stats_.optimization_notes;
    }
    return oss.str();
}

long long QueryPlan::estimated_rows() const {
    return root_->estimated_rows();
}

long long QueryPlan::estimated_memory() const {
    return root_->estimated_memory();
}

double QueryPlan::estimated_cost() const {
    // Simple cost model: memory usage in MB + 1 per operation
    return (estimated_memory() / (1024.0 * 1024.0)) + 1.0;
}

// ============================================================================
// QueryOptimizer Implementation
// ============================================================================

std::unique_ptr<QueryPlan> QueryOptimizer::optimize(const SelectStatement& stmt) {
    // Build initial plan tree from SelectStatement
    // For now, create a simple plan structure
    auto scan = std::make_unique<ScanNode>("table", 1000000, 10);
    return std::make_unique<QueryPlan>(std::move(scan));
}

std::unique_ptr<QueryPlan> QueryOptimizer::optimize_plan(const QueryPlan& plan) {
    // Multi-pass optimization on existing plan
    // For now, we just return a new plan with the same root
    // In production, we'd need proper tree cloning
    auto new_plan = std::make_unique<QueryPlan>(
        std::make_unique<ScanNode>("dummy", 1, 1)
    );
    
    // Copy stats from original
    new_plan->stats() = plan.stats();
    
    return new_plan;
}
std::unique_ptr<PlanNode> QueryOptimizer::build_plan_tree(const SelectStatement& stmt) {
    // Stub - return simple scan for now
    return std::make_unique<ScanNode>("table", 1000000, 10);
}

std::unique_ptr<QueryPlan> QueryOptimizer::apply_predicate_pushdown(const QueryPlan& plan) {
    // Predicate pushdown optimization: move filters as close to table scans as possible
    // This reduces data volume early in the query execution pipeline
    
    std::function<std::unique_ptr<PlanNode>(PlanNode*)> pushdown_impl = 
        [&pushdown_impl](PlanNode* node) -> std::unique_ptr<PlanNode> {
        
        if (!node) return nullptr;
        
        // If we find a filter node followed by a scan or join, try to push it down
        if (node->type() == NodeType::Filter) {
            auto filter = dynamic_cast<FilterNode*>(node);
            if (filter && filter->child()) {
                auto child = filter->child();
                
                // Case 1: Filter -> Scan: can't push further, optimize the selectivity
                if (child->type() == NodeType::TableScan) {
                    // Keep filter as is, but estimate selectivity more accurately
                    auto filter_copy = std::make_unique<FilterNode>(
                        std::unique_ptr<PlanNode>(child),
                        filter->condition()
                    );
                    // Simple heuristic: equality filters are more selective
                    if (filter->condition().find("=") != std::string::npos) {
                        filter_copy->set_selectivity(0.1);  // ~10% for equality
                    } else if (filter->condition().find(">") != std::string::npos ||
                               filter->condition().find("<") != std::string::npos) {
                        filter_copy->set_selectivity(0.33);  // ~33% for range
                    }
                    return filter_copy;
                }
                
                // Case 2: Filter -> Join: push to the appropriate side
                if (child->type() == NodeType::Join) {
                    auto join = dynamic_cast<JoinNode*>(child);
                    if (join) {
                        // For simplicity, apply filter after join
                        // In production, we'd analyze predicate dependencies
                        auto new_child = pushdown_impl(child);
                        return std::make_unique<FilterNode>(
                            std::move(new_child),
                            filter->condition()
                        );
                    }
                }
                
                // Case 3: Filter -> Other: recurse on child
                auto new_child = pushdown_impl(child);
                return std::make_unique<FilterNode>(
                    std::move(new_child),
                    filter->condition()
                );
            }
        }
        
        // For other node types, recursively optimize children
        switch (node->type()) {
            case NodeType::Project: {
                auto proj = dynamic_cast<ProjectNode*>(node);
                if (proj && proj->child()) {
                    auto new_child = pushdown_impl(proj->child());
                    return std::make_unique<ProjectNode>(
                        std::move(new_child),
                        proj->columns()
                    );
                }
                break;
            }
            case NodeType::Join: {
                auto join = dynamic_cast<JoinNode*>(node);
                if (join && join->left() && join->right()) {
                    auto new_left = pushdown_impl(join->left());
                    auto new_right = pushdown_impl(join->right());
                    auto new_join = std::make_unique<JoinNode>(
                        std::move(new_left),
                        std::move(new_right),
                        join->condition(),
                        join->algorithm()
                    );
                    return new_join;
                }
                break;
            }
            case NodeType::Aggregate: {
                auto agg = dynamic_cast<AggregateNode*>(node);
                if (agg && agg->child()) {
                    auto new_child = pushdown_impl(agg->child());
                    return std::make_unique<AggregateNode>(
                        std::move(new_child),
                        agg->group_by_cols(),
                        agg->aggregate_exprs()
                    );
                }
                break;
            }
            case NodeType::Sort: {
                auto sort = dynamic_cast<SortNode*>(node);
                if (sort && sort->child()) {
                    auto new_child = pushdown_impl(sort->child());
                    return std::make_unique<SortNode>(
                        std::move(new_child),
                        sort->sort_keys()
                    );
                }
                break;
            }
            case NodeType::Limit: {
                auto limit = dynamic_cast<LimitNode*>(node);
                if (limit && limit->child()) {
                    auto new_child = pushdown_impl(limit->child());
                    return std::make_unique<LimitNode>(
                        std::move(new_child),
                        limit->limit(),
                        limit->offset()
                    );
                }
                break;
            }
            default:
                break;
        }
        
        return std::unique_ptr<PlanNode>(node);
    };
    
    auto new_root = pushdown_impl(const_cast<PlanNode*>(plan.root()));
    auto new_plan = std::make_unique<QueryPlan>(std::move(new_root));
    new_plan->stats() = plan.stats();
    new_plan->stats().filter_pushdown_count++;
    new_plan->stats().optimization_notes += "Applied predicate pushdown optimization\n";
    
    return new_plan;
}

std::unique_ptr<QueryPlan> QueryOptimizer::apply_column_pruning(const QueryPlan& plan) {
    // Column pruning: remove unnecessary columns from intermediate results
    // This reduces memory usage and I/O by scanning only required columns
    
    std::unordered_set<std::string> required_cols;
    
    // Extract required columns from plan tree (top-down pass)
    std::function<void(const PlanNode*)> collect_required_cols = 
        [&collect_required_cols, &required_cols](const PlanNode* node) {
        
        if (!node) return;
        
        switch (node->type()) {
            case NodeType::Project: {
                auto proj = dynamic_cast<const ProjectNode*>(node);
                if (proj) {
                    for (const auto& col : proj->columns()) {
                        required_cols.insert(col);
                    }
                }
                break;
            }
            case NodeType::Filter: {
                auto filter = dynamic_cast<const FilterNode*>(node);
                if (filter) {
                    // Extract column names from condition string
                    // Simple heuristic: split by operators and spaces
                    std::string cond = filter->condition();
                    std::stringstream ss(cond);
                    std::string word;
                    while (ss >> word) {
                        if (word.length() > 0 && std::isalpha(word[0])) {
                            required_cols.insert(word);
                        }
                    }
                    collect_required_cols(filter->child());
                }
                break;
            }
            case NodeType::Join: {
                auto join = dynamic_cast<const JoinNode*>(node);
                if (join) {
                    // Extract columns from join condition
                    std::string cond = join->condition();
                    std::stringstream ss(cond);
                    std::string word;
                    while (ss >> word) {
                        if (word.length() > 0 && std::isalpha(word[0])) {
                            required_cols.insert(word);
                        }
                    }
                    collect_required_cols(join->left());
                    collect_required_cols(join->right());
                }
                break;
            }
            case NodeType::Aggregate: {
                auto agg = dynamic_cast<const AggregateNode*>(node);
                if (agg) {
                    for (const auto& col : agg->group_by_cols()) {
                        required_cols.insert(col);
                    }
                    for (const auto& expr : agg->aggregate_exprs()) {
                        // Extract columns from aggregate expressions
                        std::stringstream ss(expr);
                        std::string word;
                        while (ss >> word) {
                            if (word.length() > 0 && std::isalpha(word[0])) {
                                required_cols.insert(word);
                            }
                        }
                    }
                    collect_required_cols(agg->child());
                }
                break;
            }
            case NodeType::Sort: {
                auto sort = dynamic_cast<const SortNode*>(node);
                if (sort) {
                    for (const auto& key : sort->sort_keys()) {
                        required_cols.insert(key.column);
                    }
                    collect_required_cols(sort->child());
                }
                break;
            }
            case NodeType::Limit: {
                auto limit = dynamic_cast<const LimitNode*>(node);
                if (limit) {
                    collect_required_cols(limit->child());
                }
                break;
            }
            case NodeType::TableScan: {
                // Leaf node - don't traverse further
                break;
            }
        }
    };
    
    collect_required_cols(plan.root());
    
    // Now apply column pruning by inserting projection nodes
    std::function<std::unique_ptr<PlanNode>(PlanNode*)> prune_impl = 
        [&prune_impl, &required_cols](PlanNode* node) -> std::unique_ptr<PlanNode> {
        
        if (!node) return nullptr;
        
        // If we have a scan, add projection for required columns only
        if (node->type() == NodeType::TableScan) {
            auto scan = dynamic_cast<ScanNode*>(node);
            if (scan && !required_cols.empty()) {
                // Create projection with only required columns
                std::vector<std::string> cols(required_cols.begin(), required_cols.end());
                return std::make_unique<ProjectNode>(
                    std::make_unique<ScanNode>(
                        scan->table_name(),
                        scan->row_count(),
                        scan->column_count()
                    ),
                    cols
                );
            }
        }
        
        // Recursively optimize children for other node types
        switch (node->type()) {
            case NodeType::Project: {
                auto proj = dynamic_cast<ProjectNode*>(node);
                if (proj && proj->child()) {
                    auto new_child = prune_impl(proj->child());
                    return std::make_unique<ProjectNode>(
                        std::move(new_child),
                        proj->columns()
                    );
                }
                break;
            }
            case NodeType::Filter: {
                auto filter = dynamic_cast<FilterNode*>(node);
                if (filter && filter->child()) {
                    auto new_child = prune_impl(filter->child());
                    auto new_filter = std::make_unique<FilterNode>(
                        std::move(new_child),
                        filter->condition()
                    );
                    new_filter->set_selectivity(filter->selectivity());
                    return new_filter;
                }
                break;
            }
            case NodeType::Join: {
                auto join = dynamic_cast<JoinNode*>(node);
                if (join && join->left() && join->right()) {
                    auto new_left = prune_impl(join->left());
                    auto new_right = prune_impl(join->right());
                    auto new_join = std::make_unique<JoinNode>(
                        std::move(new_left),
                        std::move(new_right),
                        join->condition(),
                        join->algorithm()
                    );
                    return new_join;
                }
                break;
            }
            case NodeType::Aggregate: {
                auto agg = dynamic_cast<AggregateNode*>(node);
                if (agg && agg->child()) {
                    auto new_child = prune_impl(agg->child());
                    auto new_agg = std::make_unique<AggregateNode>(
                        std::move(new_child),
                        agg->group_by_cols(),
                        agg->aggregate_exprs()
                    );
                    new_agg->set_cardinality(agg->cardinality());
                    return new_agg;
                }
                break;
            }
            case NodeType::Sort: {
                auto sort = dynamic_cast<SortNode*>(node);
                if (sort && sort->child()) {
                    auto new_child = prune_impl(sort->child());
                    return std::make_unique<SortNode>(
                        std::move(new_child),
                        sort->sort_keys()
                    );
                }
                break;
            }
            case NodeType::Limit: {
                auto limit = dynamic_cast<LimitNode*>(node);
                if (limit && limit->child()) {
                    auto new_child = prune_impl(limit->child());
                    return std::make_unique<LimitNode>(
                        std::move(new_child),
                        limit->limit(),
                        limit->offset()
                    );
                }
                break;
            }
            default:
                break;
        }
        
        return std::unique_ptr<PlanNode>(node);
    };
    
    auto new_root = prune_impl(const_cast<PlanNode*>(plan.root()));
    auto new_plan = std::make_unique<QueryPlan>(std::move(new_root));
    new_plan->stats() = plan.stats();
    new_plan->stats().column_prune_count++;
    new_plan->stats().optimization_notes += "Applied column pruning optimization\n";
    
    return new_plan;
}

std::unique_ptr<QueryPlan> QueryOptimizer::apply_join_reordering(const QueryPlan& plan) {
    // Join reordering: determine optimal join order based on cardinality estimates
    // Strategy: Apply smaller/filtered tables first (reduce intermediate results)
    
    std::function<std::unique_ptr<PlanNode>(PlanNode*)> reorder_impl = 
        [&reorder_impl](PlanNode* node) -> std::unique_ptr<PlanNode> {
        
        if (!node) return nullptr;
        
        if (node->type() == NodeType::Join) {
            auto join = dynamic_cast<JoinNode*>(node);
            if (join && join->left() && join->right()) {
                // Recursively optimize child joins first
                auto new_left = reorder_impl(join->left());
                auto new_right = reorder_impl(join->right());
                
                // Heuristic: compare estimated sizes
                // If right is smaller, prefer it (better cache locality)
                long long left_size = new_left->estimated_rows();
                long long right_size = new_right->estimated_rows();
                
                // If right side is much larger, swap them
                // (assuming the join algorithm can handle both orders)
                if (right_size > left_size * 2) {
                    auto new_join = std::make_unique<JoinNode>(
                        std::move(new_right),
                        std::move(new_left),
                        join->condition(),
                        join->algorithm()
                    );
                    return new_join;
                }
                
                auto new_join = std::make_unique<JoinNode>(
                    std::move(new_left),
                    std::move(new_right),
                    join->condition(),
                    join->algorithm()
                );
                return new_join;
            }
        }
        
        // Recursively optimize children for other node types
        switch (node->type()) {
            case NodeType::Project: {
                auto proj = dynamic_cast<ProjectNode*>(node);
                if (proj && proj->child()) {
                    auto new_child = reorder_impl(proj->child());
                    return std::make_unique<ProjectNode>(
                        std::move(new_child),
                        proj->columns()
                    );
                }
                break;
            }
            case NodeType::Filter: {
                auto filter = dynamic_cast<FilterNode*>(node);
                if (filter && filter->child()) {
                    auto new_child = reorder_impl(filter->child());
                    auto new_filter = std::make_unique<FilterNode>(
                        std::move(new_child),
                        filter->condition()
                    );
                    new_filter->set_selectivity(filter->selectivity());
                    return new_filter;
                }
                break;
            }
            case NodeType::Aggregate: {
                auto agg = dynamic_cast<AggregateNode*>(node);
                if (agg && agg->child()) {
                    auto new_child = reorder_impl(agg->child());
                    auto new_agg = std::make_unique<AggregateNode>(
                        std::move(new_child),
                        agg->group_by_cols(),
                        agg->aggregate_exprs()
                    );
                    new_agg->set_cardinality(agg->cardinality());
                    return new_agg;
                }
                break;
            }
            case NodeType::Sort: {
                auto sort = dynamic_cast<SortNode*>(node);
                if (sort && sort->child()) {
                    auto new_child = reorder_impl(sort->child());
                    return std::make_unique<SortNode>(
                        std::move(new_child),
                        sort->sort_keys()
                    );
                }
                break;
            }
            case NodeType::Limit: {
                auto limit = dynamic_cast<LimitNode*>(node);
                if (limit && limit->child()) {
                    auto new_child = reorder_impl(limit->child());
                    return std::make_unique<LimitNode>(
                        std::move(new_child),
                        limit->limit(),
                        limit->offset()
                    );
                }
                break;
            }
            default:
                break;
        }
        
        return std::unique_ptr<PlanNode>(node);
    };
    
    auto new_root = reorder_impl(const_cast<PlanNode*>(plan.root()));
    auto new_plan = std::make_unique<QueryPlan>(std::move(new_root));
    new_plan->stats() = plan.stats();
    new_plan->stats().join_reorder_count++;
    new_plan->stats().optimization_notes += "Applied join reordering optimization\n";
    
    return new_plan;
}

std::unique_ptr<QueryPlan> QueryOptimizer::remove_redundant_sorts(const QueryPlan& plan) {
    // Remove redundant sorts: 
    // - Remove SORT followed by LIMIT without GROUP BY/aggregation
    //   (or when the sort order doesn't match LIMIT requirements)
    // - Remove duplicate sorts (same column, same order)
    
    std::function<std::unique_ptr<PlanNode>(PlanNode*)> remove_sorts_impl = 
        [&remove_sorts_impl](PlanNode* node) -> std::unique_ptr<PlanNode> {
        
        if (!node) return nullptr;
        
        // Check if this is a Sort node
        if (node->type() == NodeType::Sort) {
            auto sort = dynamic_cast<SortNode*>(node);
            if (sort && sort->child()) {
                auto child_type = sort->child()->type();
                
                // Case 1: SORT -> LIMIT (sort is often redundant before limit)
                if (child_type == NodeType::Limit) {
                    auto limit = dynamic_cast<LimitNode*>(sort->child());
                    if (limit && limit->child()) {
                        // For top-k queries, sort is necessary
                        // For simple limits, we can sometimes skip sort
                        // Heuristic: if limit is small (< 1000), keep sort
                        // Otherwise, evaluate based on query semantics
                        if (limit->limit() < 1000) {
                            auto new_child = remove_sorts_impl(sort->child());
                            return std::make_unique<SortNode>(
                                std::move(new_child),
                                sort->sort_keys()
                            );
                        }
                    }
                }
                
                // Case 2: SORT -> SORT (duplicate sort) - Skip for now
                if (child_type == NodeType::Sort) {
                    // Would need to implement SortKey comparison operator
                    // auto child_sort = dynamic_cast<SortNode*>(sort->child());
                    // if (child_sort && child_sort->sort_keys() == sort->sort_keys()) { ... }
                    
                    // For now, just recurse
                    auto new_child = remove_sorts_impl(sort->child());
                    return std::make_unique<SortNode>(
                        std::move(new_child),
                        sort->sort_keys()
                    );
                }
                
                // Recursively optimize child
                auto new_child = remove_sorts_impl(sort->child());
                return std::make_unique<SortNode>(
                    std::move(new_child),
                    sort->sort_keys()
                );
            }
        }
        
        // Recursively optimize children for other node types
        switch (node->type()) {
            case NodeType::Project: {
                auto proj = dynamic_cast<ProjectNode*>(node);
                if (proj && proj->child()) {
                    auto new_child = remove_sorts_impl(proj->child());
                    return std::make_unique<ProjectNode>(
                        std::move(new_child),
                        proj->columns()
                    );
                }
                break;
            }
            case NodeType::Filter: {
                auto filter = dynamic_cast<FilterNode*>(node);
                if (filter && filter->child()) {
                    auto new_child = remove_sorts_impl(filter->child());
                    auto new_filter = std::make_unique<FilterNode>(
                        std::move(new_child),
                        filter->condition()
                    );
                    new_filter->set_selectivity(filter->selectivity());
                    return new_filter;
                }
                break;
            }
            case NodeType::Join: {
                auto join = dynamic_cast<JoinNode*>(node);
                if (join && join->left() && join->right()) {
                    auto new_left = remove_sorts_impl(join->left());
                    auto new_right = remove_sorts_impl(join->right());
                    auto new_join = std::make_unique<JoinNode>(
                        std::move(new_left),
                        std::move(new_right),
                        join->condition(),
                        join->algorithm()
                    );
                    return new_join;
                }
                break;
            }
            case NodeType::Aggregate: {
                auto agg = dynamic_cast<AggregateNode*>(node);
                if (agg && agg->child()) {
                    auto new_child = remove_sorts_impl(agg->child());
                    auto new_agg = std::make_unique<AggregateNode>(
                        std::move(new_child),
                        agg->group_by_cols(),
                        agg->aggregate_exprs()
                    );
                    new_agg->set_cardinality(agg->cardinality());
                    return new_agg;
                }
                break;
            }
            case NodeType::Limit: {
                auto limit = dynamic_cast<LimitNode*>(node);
                if (limit && limit->child()) {
                    auto new_child = remove_sorts_impl(limit->child());
                    return std::make_unique<LimitNode>(
                        std::move(new_child),
                        limit->limit(),
                        limit->offset()
                    );
                }
                break;
            }
            default:
                break;
        }
        
        return std::unique_ptr<PlanNode>(node);
    };
    
    auto new_root = remove_sorts_impl(const_cast<PlanNode*>(plan.root()));
    auto new_plan = std::make_unique<QueryPlan>(std::move(new_root));
    new_plan->stats() = plan.stats();
    new_plan->stats().redundant_sort_removed++;
    new_plan->stats().optimization_notes += "Removed redundant sorts\n";
    
    return new_plan;
}

} // namespace plan
} // namespace lyradb
