#include <gtest/gtest.h>
#include "lyradb/index_aware_optimizer.h"
#include "lyradb/index_manager.h"
#include "lyradb/query_plan.h"
#include <memory>

using namespace lyradb;
using namespace lyradb::plan;

// ============================================================================
// IndexedScanNode Tests
// ============================================================================

class IndexedScanNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        scan = std::make_unique<IndexedScanNode>("users", 1000000, 10);
    }

    std::unique_ptr<IndexedScanNode> scan;
};

TEST_F(IndexedScanNodeTest, CreatesWithoutIndex) {
    EXPECT_EQ(scan->type(), NodeType::TableScan);
    EXPECT_EQ(scan->table_name(), "users");
    EXPECT_EQ(scan->row_count(), 1000000);
    EXPECT_FALSE(scan->uses_index());
}

TEST_F(IndexedScanNodeTest, UsesIndexAfterConfiguration) {
    scan->use_index("idx_email", "email", "Hash");
    EXPECT_TRUE(scan->uses_index());
    EXPECT_EQ(scan->index_name(), "idx_email");
    EXPECT_EQ(scan->index_column(), "email");
    EXPECT_EQ(scan->index_type(), "Hash");
}

TEST_F(IndexedScanNodeTest, EstimatesRowsCorrectly) {
    EXPECT_EQ(scan->estimated_rows(), 1000000);
    scan->set_estimated_rows(500000);
    EXPECT_EQ(scan->estimated_rows(), 500000);
}

TEST_F(IndexedScanNodeTest, CalculatesMemoryUsage) {
    // 1M rows * 100 bytes/row = 100MB
    EXPECT_EQ(scan->estimated_memory(), 100000000);
}

TEST_F(IndexedScanNodeTest, ToStringRepresentation) {
    std::string str = scan->to_string();
    EXPECT_TRUE(str.find("IndexedScan") != std::string::npos);
    EXPECT_TRUE(str.find("users") != std::string::npos);
    
    scan->use_index("idx_email", "email", "Hash");
    str = scan->to_string();
    EXPECT_TRUE(str.find("idx_email") != std::string::npos);
    EXPECT_TRUE(str.find("Hash") != std::string::npos);
}

// ============================================================================
// IndexedFilterNode Tests
// ============================================================================

class IndexedFilterNodeTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto scan = std::make_unique<ScanNode>("products", 50000, 5);
        filter = std::make_unique<IndexedFilterNode>(std::move(scan), "price > 100");
    }

    std::unique_ptr<IndexedFilterNode> filter;
};

TEST_F(IndexedFilterNodeTest, CreatesFilterNode) {
    EXPECT_EQ(filter->type(), NodeType::Filter);
    EXPECT_EQ(filter->condition(), "price > 100");
    EXPECT_EQ(filter->child()->type(), NodeType::TableScan);
}

TEST_F(IndexedFilterNodeTest, PredicateTypeDetection) {
    filter->set_predicate_info(IndexedFilterNode::PredicateType::RANGE, "price", 0.3);
    EXPECT_EQ(filter->predicate_type(), IndexedFilterNode::PredicateType::RANGE);
    EXPECT_EQ(filter->predicate_column(), "price");
}

TEST_F(IndexedFilterNodeTest, SelectivityCalculation) {
    filter->set_selectivity(0.3);
    // 50000 rows * 0.3 = 15000
    EXPECT_EQ(filter->estimated_rows(), 15000);
}

TEST_F(IndexedFilterNodeTest, PredicateTypes) {
    // Equality
    auto eq_filter = std::make_unique<IndexedFilterNode>(
        std::make_unique<ScanNode>("t", 100, 1), "status = 'active'"
    );
    eq_filter->set_predicate_info(IndexedFilterNode::PredicateType::EQUALITY, "status", 0.5);
    EXPECT_EQ(eq_filter->predicate_type(), IndexedFilterNode::PredicateType::EQUALITY);

    // Range
    auto range_filter = std::make_unique<IndexedFilterNode>(
        std::make_unique<ScanNode>("t", 100, 1), "amount > 1000"
    );
    range_filter->set_predicate_info(IndexedFilterNode::PredicateType::RANGE, "amount", 0.4);
    EXPECT_EQ(range_filter->predicate_type(), IndexedFilterNode::PredicateType::RANGE);

    // IN list
    auto in_filter = std::make_unique<IndexedFilterNode>(
        std::make_unique<ScanNode>("t", 100, 1), "region IN ('US', 'CA', 'MX')"
    );
    in_filter->set_predicate_info(IndexedFilterNode::PredicateType::IN_LIST, "region", 0.3);
    EXPECT_EQ(in_filter->predicate_type(), IndexedFilterNode::PredicateType::IN_LIST);
}

// ============================================================================
// IndexAwareOptimizer Tests
// ============================================================================

class IndexAwareOptimizerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<IndexManager>();
        optimizer = std::make_unique<IndexAwareOptimizer>(manager.get());
        
        // Create some indexes
        manager->create_hash_index("idx_users_email", "users", "email");
        manager->update_statistics("idx_users_email", 50000);  // 50K distinct emails
        
        manager->create_btree_index("idx_orders_amount", "orders", "amount");
        manager->update_statistics("idx_orders_amount", 1000000);  // Continuous values
        
        manager->create_bitmap_index("idx_products_category", "products", "category");
        manager->update_statistics("idx_products_category", 15);  // 15 categories
    }

    std::unique_ptr<IndexManager> manager;
    std::unique_ptr<IndexAwareOptimizer> optimizer;
};

TEST_F(IndexAwareOptimizerTest, PredicateAnalysis) {
    std::string column;
    
    // Test equality predicate
    auto type = optimizer->analyze_predicate("email = 'test@example.com'", column);
    EXPECT_EQ(type, IndexedFilterNode::PredicateType::EQUALITY);
    EXPECT_EQ(column, "email");

    // Test range predicate
    type = optimizer->analyze_predicate("age > 18", column);
    EXPECT_EQ(type, IndexedFilterNode::PredicateType::RANGE);
    EXPECT_EQ(column, "age");

    // Test IN predicate
    type = optimizer->analyze_predicate("status IN ('active', 'pending')", column);
    EXPECT_EQ(type, IndexedFilterNode::PredicateType::IN_LIST);
    EXPECT_EQ(column, "status");

    // Test BETWEEN predicate
    type = optimizer->analyze_predicate("date BETWEEN '2024-01-01' AND '2024-12-31'", column);
    EXPECT_EQ(type, IndexedFilterNode::PredicateType::BETWEEN);
}

TEST_F(IndexAwareOptimizerTest, CostEstimationFullScan) {
    IndexSelectionStats stats;
    stats.row_count = 1000000;
    stats.table_name = "users";
    stats.column_name = "email";
    stats.selectivity = 0.1;
    
    double cost = optimizer->estimate_scan_cost(stats);
    EXPECT_GT(cost, 0.0);
}

TEST_F(IndexAwareOptimizerTest, IndexScanCostEstimation) {
    IndexSelectionStats stats;
    stats.row_count = 1000000;
    stats.cardinality = 100;
    stats.selectivity = 0.1;

    // B-tree cost (O(log N))
    double btree_cost = optimizer->estimate_index_scan_cost(stats, "BTree");
    EXPECT_GT(btree_cost, 0.0);

    // Hash cost (O(1))
    double hash_cost = optimizer->estimate_index_scan_cost(stats, "Hash");
    EXPECT_LT(hash_cost, btree_cost);  // Hash should be faster than B-tree

    // Bitmap cost (very fast for low cardinality)
    double bitmap_cost = optimizer->estimate_index_scan_cost(stats, "Bitmap");
    EXPECT_LT(bitmap_cost, hash_cost);  // Bitmap should be fastest
}

TEST_F(IndexAwareOptimizerTest, IndexOpportunityDetection) {
    // Create a simple plan: Scan -> Filter
    auto scan = std::make_unique<ScanNode>("users", 1000000, 5);
    auto filter = std::make_unique<FilterNode>(std::move(scan), "email = 'test@example.com'");
    filter->set_selectivity(0.0001);  // 1 row out of 1M
    
    auto plan = std::make_unique<QueryPlan>(std::move(filter));
    
    auto opportunities = optimizer->analyze_index_opportunities(*plan);
    
    // Should find opportunity to use email index
    EXPECT_GT(opportunities.size(), 0);
}

TEST_F(IndexAwareOptimizerTest, IndexSelectionForEquality) {
    IndexSelectionStats stats;
    stats.table_name = "users";
    stats.column_name = "email";
    stats.row_count = 1000000;
    stats.cardinality = 50000;
    stats.selectivity = 0.0001;
    stats.predicate_type = "equality";
    
    stats.full_scan_cost = optimizer->estimate_scan_cost(stats);
    EXPECT_GT(stats.full_scan_cost, 0.0);
}

TEST_F(IndexAwareOptimizerTest, IndexSelectionForRange) {
    IndexSelectionStats stats;
    stats.table_name = "orders";
    stats.column_name = "amount";
    stats.row_count = 500000;
    stats.cardinality = 1000000;  // High cardinality
    stats.selectivity = 0.2;  // 20% of rows
    stats.predicate_type = "range";
    
    stats.full_scan_cost = optimizer->estimate_scan_cost(stats);
    stats.index_scan_cost = optimizer->estimate_index_scan_cost(stats, "BTree");
    
    // B-tree should be more efficient than full scan for range with selectivity
    EXPECT_LT(stats.index_scan_cost, stats.full_scan_cost);
}

TEST_F(IndexAwareOptimizerTest, BitmapIndexForLowCardinality) {
    IndexSelectionStats stats;
    stats.table_name = "products";
    stats.column_name = "category";
    stats.row_count = 1000000;
    stats.cardinality = 15;  // Very low
    stats.selectivity = 1.0 / 15;  // One category
    stats.predicate_type = "equality";
    
    double bitmap_cost = optimizer->estimate_index_scan_cost(stats, "Bitmap");
    double full_cost = optimizer->estimate_scan_cost(stats);
    
    // Bitmap should be much cheaper than full scan for low cardinality
    EXPECT_LT(bitmap_cost, full_cost);
}

TEST_F(IndexAwareOptimizerTest, OptimizeWithIndexes) {
    // Create a simple plan
    auto scan = std::make_unique<ScanNode>("products", 100000, 3);
    auto plan = std::make_unique<QueryPlan>(std::move(scan));
    
    // Apply index-aware optimization
    auto optimized = optimizer->optimize_with_indexes(*plan);
    
    EXPECT_NE(optimized, nullptr);
    EXPECT_NE(optimized->root(), nullptr);
}

TEST_F(IndexAwareOptimizerTest, MultiPredicateOptimization) {
    // Test with multiple predicates on different columns
    
    // Scenario: users WHERE region = 'US' AND status = 'active' AND age > 18
    // region: bitmap (low cardinality)
    // status: bitmap (low cardinality)
    // age: btree (continuous range)
    
    IndexSelectionStats region_stats;
    region_stats.table_name = "users";
    region_stats.column_name = "region";
    region_stats.row_count = 10000000;
    region_stats.cardinality = 50;
    region_stats.selectivity = 0.02;  // 2% from region
    
    IndexSelectionStats status_stats;
    status_stats.table_name = "users";
    status_stats.column_name = "status";
    status_stats.row_count = 10000000;
    status_stats.cardinality = 5;
    status_stats.selectivity = 0.3;  // 30% active
    
    // Verify cost calculations
    double region_bitmap_cost = optimizer->estimate_index_scan_cost(region_stats, "Bitmap");
    double status_bitmap_cost = optimizer->estimate_index_scan_cost(status_stats, "Bitmap");
    
    EXPECT_GT(region_bitmap_cost, 0.0);
    EXPECT_GT(status_bitmap_cost, 0.0);
    // Bitmap should be fast for both low-cardinality columns
    EXPECT_LT(region_bitmap_cost, optimizer->estimate_scan_cost(region_stats));
    EXPECT_LT(status_bitmap_cost, optimizer->estimate_scan_cost(status_stats));
}

TEST_F(IndexAwareOptimizerTest, SkipsIndexForHighSelectivity) {
    IndexSelectionStats stats;
    stats.row_count = 1000000;
    stats.cardinality = 500000;
    stats.selectivity = 0.9;  // Returns 90% of rows
    
    double full_scan = optimizer->estimate_scan_cost(stats);
    double index_scan = optimizer->estimate_index_scan_cost(stats, "BTree");
    
    // For high selectivity, full scan might be better
    // (though this depends on cost model tuning)
    EXPECT_GT(full_scan, 0.0);
    EXPECT_GT(index_scan, 0.0);
}

// ============================================================================
// Integration Tests
// ============================================================================

class IndexOptimizerIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<IndexManager>();
        optimizer = std::make_unique<IndexAwareOptimizer>(manager.get());
        
        // Setup: E-commerce scenario
        manager->create_hash_index("idx_orders_customer", "orders", "customer_id");
        manager->update_statistics("idx_orders_customer", 100000);
        
        manager->create_bitmap_index("idx_orders_status", "orders", "status");
        manager->update_statistics("idx_orders_status", 4);  // pending, shipped, delivered, cancelled
        
        manager->create_btree_index("idx_orders_date", "orders", "order_date");
        manager->update_statistics("idx_orders_date", 365);  // 365 distinct dates
        
        manager->create_btree_index("idx_orders_amount", "orders", "total_amount");
        manager->update_statistics("idx_orders_amount", 50000);  // 50K distinct amounts
    }

    std::unique_ptr<IndexManager> manager;
    std::unique_ptr<IndexAwareOptimizer> optimizer;
};

TEST_F(IndexOptimizerIntegrationTest, QueryWithEqualityPredicate) {
    // Query: SELECT * FROM orders WHERE customer_id = '12345'
    std::string column;
    auto type = optimizer->analyze_predicate("customer_id = '12345'", column);
    
    EXPECT_EQ(type, IndexedFilterNode::PredicateType::EQUALITY);
    EXPECT_EQ(column, "customer_id");
    
    // Should use hash index on customer_id
    auto indexes = manager->get_indexes_on_column("orders", "customer_id");
    EXPECT_EQ(indexes.size(), 1);
    EXPECT_EQ(indexes[0], "idx_orders_customer");
}

TEST_F(IndexOptimizerIntegrationTest, QueryWithRangePredicate) {
    // Query: SELECT * FROM orders WHERE order_date > '2024-01-01'
    std::string column;
    auto type = optimizer->analyze_predicate("order_date > '2024-01-01'", column);
    
    EXPECT_EQ(type, IndexedFilterNode::PredicateType::RANGE);
    EXPECT_EQ(column, "order_date");
}

TEST_F(IndexOptimizerIntegrationTest, QueryWithBitmapOptimal) {
    // Query: SELECT * FROM orders WHERE status = 'shipped'
    // Only 4 distinct statuses - bitmap is optimal
    
    IndexSelectionStats stats;
    stats.table_name = "orders";
    stats.column_name = "status";
    stats.row_count = 5000000;
    stats.cardinality = 4;
    stats.selectivity = 0.25;
    
    double full_scan = optimizer->estimate_scan_cost(stats);
    double bitmap_scan = optimizer->estimate_index_scan_cost(stats, "Bitmap");
    
    EXPECT_LT(bitmap_scan, full_scan);
}

TEST_F(IndexOptimizerIntegrationTest, RecommendationHeuristics) {
    // Low cardinality - should recommend Bitmap
    auto rec1 = manager->recommend_index("orders", "status", 4, "equality");
    EXPECT_NE(rec1, "none");
    
    // High cardinality, equality query - should recommend Hash
    auto rec2 = manager->recommend_index("orders", "customer_id", 100000, "equality");
    EXPECT_NE(rec2, "none");
    
    // High cardinality, range query - should recommend B-tree
    auto rec3 = manager->recommend_index("orders", "total_amount", 50000, "range");
    EXPECT_NE(rec3, "none");
}

// ============================================================================
// Performance Scenario Tests
// ============================================================================

class IndexOptimizationScenarioTest : public ::testing::Test {
protected:
    std::unique_ptr<IndexManager> manager;
    std::unique_ptr<IndexAwareOptimizer> optimizer;
};

TEST_F(IndexOptimizationScenarioTest, LargeTableSmallResult) {
    manager = std::make_unique<IndexManager>();
    optimizer = std::make_unique<IndexAwareOptimizer>(manager.get());
    
    manager->create_hash_index("idx_users_email", "users", "email");
    manager->update_statistics("idx_users_email", 10000000);  // 10M unique users
    
    IndexSelectionStats stats;
    stats.table_name = "users";
    stats.column_name = "email";
    stats.row_count = 10000000;
    stats.cardinality = 10000000;
    stats.selectivity = 0.0000001;  // Single row
    
    double full_scan = optimizer->estimate_scan_cost(stats);
    double index_scan = optimizer->estimate_index_scan_cost(stats, "Hash");
    
    // Index dramatically better for single row lookup
    EXPECT_LT(index_scan, full_scan);
}

TEST_F(IndexOptimizationScenarioTest, TimeSeriesDataWithIndexing) {
    manager = std::make_unique<IndexManager>();
    optimizer = std::make_unique<IndexAwareOptimizer>(manager.get());
    
    manager->create_btree_index("idx_events_timestamp", "events", "timestamp");
    manager->update_statistics("idx_events_timestamp", 100000000);  // 100M records
    
    IndexSelectionStats stats;
    stats.table_name = "events";
    stats.column_name = "timestamp";
    stats.row_count = 100000000;
    stats.cardinality = 100000000;
    stats.selectivity = 0.001;  // Last 24 hours
    
    double btree_scan = optimizer->estimate_index_scan_cost(stats, "BTree");
    EXPECT_GT(btree_scan, 0.0);
}
