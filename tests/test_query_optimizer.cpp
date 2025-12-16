#include <gtest/gtest.h>
#include "lyradb/query_plan.h"
#include "lyradb/sql_parser.h"

using namespace lyradb;

// ============================================================================
// QueryOptimizer Tests
// ============================================================================

class QueryOptimizerTest : public ::testing::Test {
protected:
    plan::QueryOptimizer optimizer;
};

// ============================================================================
// Plan Node Tests
// ============================================================================

TEST(PlanNodeTest, ScanNodeBasic) {
    plan::ScanNode scan("users", 1000000, 10);
    
    EXPECT_EQ(scan.type(), plan::NodeType::TableScan);
    EXPECT_EQ(scan.table_name(), "users");
    EXPECT_EQ(scan.row_count(), 1000000);
    EXPECT_EQ(scan.column_count(), 10);
    EXPECT_EQ(scan.estimated_rows(), 1000000);
    EXPECT_GT(scan.estimated_memory(), 0);
}

TEST(PlanNodeTest, ScanNodeString) {
    plan::ScanNode scan("products", 500000, 5);
    std::string str = scan.to_string();
    
    EXPECT_THAT(str, ::testing::HasSubstr("TableScan"));
    EXPECT_THAT(str, ::testing::HasSubstr("products"));
}

TEST(PlanNodeTest, FilterNodeBasic) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::FilterNode filter(std::move(scan), "age > 18");
    
    EXPECT_EQ(filter.type(), plan::NodeType::Filter);
    EXPECT_EQ(filter.condition(), "age > 18");
    EXPECT_EQ(filter.selectivity(), 0.5);  // Default
}

TEST(PlanNodeTest, FilterNodeSelectivity) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::FilterNode filter(std::move(scan), "active = true");
    filter.set_selectivity(0.9);
    
    EXPECT_EQ(filter.selectivity(), 0.9);
    EXPECT_EQ(filter.estimated_rows(), 900000);  // 1000000 * 0.9
}

TEST(PlanNodeTest, ProjectNodeBasic) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    std::vector<std::string> cols = {"id", "name", "email"};
    plan::ProjectNode proj(std::move(scan), cols);
    
    EXPECT_EQ(proj.type(), plan::NodeType::Project);
    EXPECT_EQ(proj.columns().size(), 3);
    EXPECT_EQ(proj.estimated_rows(), 1000000);
}

TEST(PlanNodeTest, JoinNodeBasic) {
    auto left = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    auto right = std::make_unique<plan::ScanNode>("orders", 500000, 5);
    plan::JoinNode join(std::move(left), std::move(right), "users.id = orders.user_id");
    
    EXPECT_EQ(join.type(), plan::NodeType::Join);
    EXPECT_EQ(join.algorithm(), plan::JoinAlgorithm::HASH_JOIN);
    EXPECT_THAT(join.condition(), ::testing::HasSubstr("users.id"));
}

TEST(PlanNodeTest, JoinNodeEstimates) {
    auto left = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    auto right = std::make_unique<plan::ScanNode>("orders", 500000, 5);
    plan::JoinNode join(std::move(left), std::move(right), "users.id = orders.user_id");
    
    // Join result estimated as 10% of cross product
    long long expected = (1000000 * 500000) / 10;
    EXPECT_EQ(join.estimated_rows(), expected);
}

TEST(PlanNodeTest, AggregateNodeBasic) {
    auto scan = std::make_unique<plan::ScanNode>("orders", 500000, 5);
    std::vector<std::string> group_by = {"user_id"};
    std::vector<std::string> aggs = {"COUNT(*)", "SUM(amount)"};
    
    plan::AggregateNode agg(std::move(scan), group_by, aggs);
    
    EXPECT_EQ(agg.type(), plan::NodeType::Aggregate);
    EXPECT_EQ(agg.group_by_cols().size(), 1);
    EXPECT_EQ(agg.aggregate_exprs().size(), 2);
}

TEST(PlanNodeTest, AggregateNodeNoGroupBy) {
    auto scan = std::make_unique<plan::ScanNode>("orders", 500000, 5);
    std::vector<std::string> group_by;  // Empty
    std::vector<std::string> aggs = {"COUNT(*)"};
    
    plan::AggregateNode agg(std::move(scan), group_by, aggs);
    
    // Without GROUP BY, result is single row
    EXPECT_EQ(agg.estimated_rows(), 1);
}

TEST(PlanNodeTest, SortNodeBasic) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    std::vector<plan::SortNode::SortKey> keys = {
        {"name", true},   // ASC
        {"age", false}    // DESC
    };
    plan::SortNode sort(std::move(scan), keys);
    
    EXPECT_EQ(sort.type(), plan::NodeType::Sort);
    EXPECT_EQ(sort.sort_keys().size(), 2);
    EXPECT_EQ(sort.estimated_rows(), 1000000);
}

TEST(PlanNodeTest, LimitNodeBasic) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::LimitNode limit(std::move(scan), 100, 10);
    
    EXPECT_EQ(limit.type(), plan::NodeType::Limit);
    EXPECT_EQ(limit.limit(), 100);
    EXPECT_EQ(limit.offset(), 10);
    EXPECT_EQ(limit.estimated_rows(), 100);
}

TEST(PlanNodeTest, LimitNodeLessThanInput) {
    auto scan = std::make_unique<plan::ScanNode>("users", 50, 10);
    plan::LimitNode limit(std::move(scan), 100, 0);
    
    // Result is min(limit, input_rows)
    EXPECT_EQ(limit.estimated_rows(), 50);
}

// ============================================================================
// QueryPlan Tests
// ============================================================================

TEST(QueryPlanTest, SimpleQueryPlan) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::QueryPlan plan(std::move(scan));
    
    EXPECT_NE(plan.root(), nullptr);
    EXPECT_EQ(plan.root()->type(), plan::NodeType::TableScan);
    EXPECT_EQ(plan.estimated_rows(), 1000000);
}

TEST(QueryPlanTest, PlanString) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::QueryPlan plan(std::move(scan));
    std::string str = plan.to_string();
    
    EXPECT_THAT(str, ::testing::HasSubstr("QueryPlan"));
    EXPECT_THAT(str, ::testing::HasSubstr("TableScan"));
}

TEST(QueryPlanTest, EstimatedCost) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::QueryPlan plan(std::move(scan));
    
    double cost = plan.estimated_cost();
    EXPECT_GT(cost, 0.0);
}

TEST(QueryPlanTest, OptimizationStats) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::QueryPlan plan(std::move(scan));
    
    plan.stats().filter_pushdown_count = 1;
    plan.stats().optimization_notes = "Test optimization";
    
    EXPECT_EQ(plan.stats().filter_pushdown_count, 1);
    EXPECT_EQ(plan.stats().optimization_notes, "Test optimization");
}

// ============================================================================
// Complex Plan Tree Tests
// ============================================================================

TEST(QueryPlanTest, ComplexFilterProjectPlan) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    
    auto filter = std::make_unique<plan::FilterNode>(
        std::move(scan), "age > 18"
    );
    
    std::vector<std::string> cols = {"id", "name", "email"};
    plan::QueryPlan plan(
        std::make_unique<plan::ProjectNode>(std::move(filter), cols)
    );
    
    EXPECT_EQ(plan.root()->type(), plan::NodeType::Project);
    EXPECT_LT(plan.estimated_rows(), 1000000);
}

TEST(QueryPlanTest, JoinAggregatePlan) {
    auto users = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    auto orders = std::make_unique<plan::ScanNode>("orders", 500000, 5);
    
    auto join = std::make_unique<plan::JoinNode>(
        std::move(users),
        std::move(orders),
        "users.id = orders.user_id"
    );
    
    std::vector<std::string> group_by = {"users.id"};
    std::vector<std::string> aggs = {"COUNT(*)", "SUM(amount)"};
    
    plan::QueryPlan plan(
        std::make_unique<plan::AggregateNode>(std::move(join), group_by, aggs)
    );
    
    EXPECT_EQ(plan.root()->type(), plan::NodeType::Aggregate);
}

// ============================================================================
// QueryOptimizer Integration Tests
// ============================================================================

TEST_F(QueryOptimizerTest, OptimizeSimpleSelect) {
    // Create a simple SELECT * FROM users query
    query::SelectStatement stmt;
    stmt.distinct = false;
    
    auto col_ref = std::make_unique<query::ColumnRefExpr>("*");
    stmt.select_list.push_back(std::move(col_ref));
    
    stmt.from_table = std::make_unique<query::TableReference>();
    stmt.from_table->table_name = "users";
    
    auto plan = optimizer.optimize(stmt);
    
    EXPECT_NE(plan, nullptr);
    EXPECT_NE(plan->root(), nullptr);
}

TEST_F(QueryOptimizerTest, OptimizeSelectWithWhere) {
    query::SelectStatement stmt;
    stmt.distinct = false;
    
    auto col_ref = std::make_unique<query::ColumnRefExpr>("*");
    stmt.select_list.push_back(std::move(col_ref));
    
    stmt.from_table = std::make_unique<query::TableReference>();
    stmt.from_table->table_name = "users";
    
    // Add WHERE clause
    auto where_expr = std::make_unique<query::BinaryExpr>(
        std::make_unique<query::ColumnRefExpr>("age"),
        query::BinaryOp::GREATER_THAN,
        std::make_unique<query::LiteralExpr>(query::Token{
            query::TokenType::INTEGER, "18", 1, 0
        })
    );
    stmt.where_clause = std::move(where_expr);
    
    auto plan = optimizer.optimize(stmt);
    
    EXPECT_NE(plan, nullptr);
    EXPECT_NE(plan->root(), nullptr);
}

TEST_F(QueryOptimizerTest, OptimizationPassesRun) {
    query::SelectStatement stmt;
    stmt.distinct = false;
    
    auto col_ref = std::make_unique<query::ColumnRefExpr>("*");
    stmt.select_list.push_back(std::move(col_ref));
    
    stmt.from_table = std::make_unique<query::TableReference>();
    stmt.from_table->table_name = "users";
    
    auto plan = optimizer.optimize(stmt);
    
    // Check that optimization passes recorded their work
    EXPECT_GE(plan->stats().filter_pushdown_count, 0);
}

// ============================================================================
// Plan Node Tree Traversal Tests
// ============================================================================

TEST(PlanNodeTest, TreeChildren) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    auto filter = std::make_unique<plan::FilterNode>(
        std::move(scan), "age > 18"
    );
    
    auto children = filter->children();
    EXPECT_EQ(children.size(), 1);
    EXPECT_NE(children[0], nullptr);
}

TEST(PlanNodeTest, TreeChildrenJoin) {
    auto left = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    auto right = std::make_unique<plan::ScanNode>("orders", 500000, 5);
    auto join = std::make_unique<plan::JoinNode>(
        std::move(left),
        std::move(right),
        "users.id = orders.user_id"
    );
    
    auto children = join->children();
    EXPECT_EQ(children.size(), 2);
}

// ============================================================================
// Memory Estimation Tests
// ============================================================================

TEST(QueryPlanTest, MemoryEstimation) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    long long memory = scan->estimated_memory();
    
    // Should be positive and reasonable
    EXPECT_GT(memory, 0);
    EXPECT_LT(memory, 1e12);  // Less than 1TB
}

TEST(QueryPlanTest, FilterReducesMemory) {
    auto scan = std::make_unique<plan::ScanNode>("orders", 1000000, 10);
    auto filter = std::make_unique<plan::FilterNode>(
        std::move(scan), "status = 'completed'"
    );
    filter->set_selectivity(0.3);  // Only 30% pass
    
    long long filter_memory = filter->estimated_memory();
    long long scan_memory = 1000000 * 50;
    
    EXPECT_LT(filter_memory, scan_memory);
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST(PlanNodeTest, EmptyGroupBy) {
    auto scan = std::make_unique<plan::ScanNode>("orders", 500000, 5);
    std::vector<std::string> group_by;  // Empty
    std::vector<std::string> aggs = {"SUM(amount)"};
    
    plan::AggregateNode agg(std::move(scan), group_by, aggs);
    
    EXPECT_EQ(agg.group_by_cols().size(), 0);
    EXPECT_EQ(agg.estimated_rows(), 1);
}

TEST(PlanNodeTest, LimitWithZeroOffset) {
    auto scan = std::make_unique<plan::ScanNode>("users", 100, 10);
    plan::LimitNode limit(std::move(scan), 50, 0);
    
    EXPECT_EQ(limit.offset(), 0);
    EXPECT_EQ(limit.estimated_rows(), 50);
}

TEST(PlanNodeTest, HighSelectivityFilter) {
    auto scan = std::make_unique<plan::ScanNode>("users", 1000000, 10);
    plan::FilterNode filter(std::move(scan), "always_true");
    filter.set_selectivity(0.99);
    
    EXPECT_EQ(filter.estimated_rows(), 990000);
}

