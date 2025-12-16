/**
 * @file test_composite_optimization.cpp
 * @brief Comprehensive tests for composite query optimization
 * 
 * Tests Phase 4.2 optimization components:
 * - Range predicate detection
 * - Selectivity estimation
 * - Cost-based index selection
 * - Speedup calculation
 * - Composite index support
 */

#include <gtest/gtest.h>
#include "lyradb/composite_query_optimizer.h"
#include <iostream>

using namespace lyradb;

class CompositeOptimizationTest : public ::testing::Test {
protected:
    CompositeQueryOptimizer optimizer;
    
    void SetUp() override {
        optimizer.reset_stats();
    }
};

// ============================================================================
// Range Predicate Detection Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, DetectBETWEENPredicate) {
    auto decision = optimizer.analyze_query(
        "users",
        "age BETWEEN 18 AND 65",
        10000,
        {}
    );
    
    EXPECT_TRUE(decision.use_index);
    EXPECT_LT(decision.estimated_selectivity, 0.5);
    EXPECT_GT(decision.estimated_speedup, 1.0);
}

TEST_F(CompositeOptimizationTest, DetectRangePredicate) {
    auto decision = optimizer.analyze_query(
        "products",
        "price > 100",
        10000,
        {}
    );
    
    EXPECT_TRUE(decision.use_index);
    EXPECT_LT(decision.estimated_selectivity, 1.0);
    EXPECT_GT(decision.estimated_speedup, 1.0);
}

TEST_F(CompositeOptimizationTest, DetectEqualityPredicate) {
    auto decision = optimizer.analyze_query(
        "orders",
        "status = 'pending'",
        10000,
        {}
    );
    
    EXPECT_TRUE(decision.use_index);
    EXPECT_LT(decision.estimated_selectivity, 0.5);
    EXPECT_GT(decision.estimated_speedup, 1.0);
}

TEST_F(CompositeOptimizationTest, RejectSmallTable) {
    auto decision = optimizer.analyze_query(
        "config",
        "id = 1",
        100,  // Below MIN_TABLE_SIZE threshold
        {}
    );
    
    EXPECT_FALSE(decision.use_index);
}

// ============================================================================
// Selectivity Estimation Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, SelectivityEqualityLow) {
    auto decision = optimizer.analyze_query(
        "users",
        "id = 12345",
        100000,
        {}
    );
    
    EXPECT_LT(decision.estimated_selectivity, 0.01);
}

TEST_F(CompositeOptimizationTest, SelectivityBETWEENModerate) {
    auto decision = optimizer.analyze_query(
        "orders",
        "order_date BETWEEN '2024-01-01' AND '2024-12-31'",
        100000,
        {}
    );
    
    EXPECT_GT(decision.estimated_selectivity, 0.05);
    EXPECT_LT(decision.estimated_selectivity, 0.2);
}

TEST_F(CompositeOptimizationTest, SelectivityRangeHigh) {
    auto decision = optimizer.analyze_query(
        "logs",
        "level > 'WARNING'",
        100000,
        {}
    );
    
    EXPECT_GT(decision.estimated_selectivity, 0.2);
}

TEST_F(CompositeOptimizationTest, SelectivityCompositeAND) {
    auto decision = optimizer.analyze_query(
        "products",
        "category = 'Electronics' AND price > 100",
        10000,
        {}
    );
    
    // AND should reduce selectivity (multiplicative)
    EXPECT_LT(decision.estimated_selectivity, 0.05);
    EXPECT_TRUE(decision.use_index);
}

TEST_F(CompositeOptimizationTest, SelectivityCompositeOR) {
    auto decision = optimizer.analyze_query(
        "products",
        "category = 'Electronics' OR category = 'Books'",
        10000,
        {}
    );
    
    // OR should increase selectivity compared to single predicate
    EXPECT_GT(decision.estimated_selectivity, 0.01);
}

// ============================================================================
// Cost-Based Index Selection Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, SelectIndexForSmallSelectivity) {
    auto decision = optimizer.analyze_query(
        "large_table",
        "id = 999999",
        1000000,
        {"idx_large_table_id"}
    );
    
    EXPECT_TRUE(decision.use_index);
    EXPECT_EQ(decision.primary_index, "idx_large_table_id");
}

TEST_F(CompositeOptimizationTest, AvoidIndexForHighSelectivity) {
    auto decision = optimizer.analyze_query(
        "table",
        "status > 'A'",
        1000,
        {"idx_table_status"}
    );
    
    // High selectivity means most rows match, index not helpful
    EXPECT_FALSE(decision.use_index);
}

TEST_F(CompositeOptimizationTest, PreferCompositeIndex) {
    auto decision = optimizer.analyze_query(
        "users",
        "country = 'USA' AND age > 18",
        50000,
        {"idx_users_composite", "idx_users_country"}
    );
    
    EXPECT_TRUE(decision.use_index);
    EXPECT_TRUE(decision.use_multiple_indexes || 
                decision.primary_index.find("composite") != std::string::npos);
}

// ============================================================================
// Speedup Calculation Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, SpeedupIncreaseWithTableSize) {
    auto small_table = optimizer.analyze_query(
        "small",
        "value > 100",
        1000,
        {}
    );
    
    auto large_table = optimizer.analyze_query(
        "large",
        "value > 100",
        1000000,
        {}
    );
    
    // Speedup should be larger for larger tables
    EXPECT_GT(large_table.estimated_speedup, small_table.estimated_speedup);
}

TEST_F(CompositeOptimizationTest, SpeedupIncreaseWithSelectivity) {
    auto high_selectivity = optimizer.analyze_query(
        "table",
        "status > 'M'",
        10000,
        {}
    );
    
    auto low_selectivity = optimizer.analyze_query(
        "table",
        "id = 12345",
        10000,
        {}
    );
    
    // Lower selectivity should have better speedup
    EXPECT_GT(low_selectivity.estimated_speedup, high_selectivity.estimated_speedup);
}

// ============================================================================
// Composite Index Support Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, CompositeIndexCreation) {
    auto decision = optimizer.analyze_query(
        "orders",
        "customer_id = 100 AND order_date > '2024-01-01'",
        50000,
        {}
    );
    
    EXPECT_TRUE(decision.use_index);
    EXPECT_TRUE(decision.use_multiple_indexes || 
                decision.primary_index.find("_composite") != std::string::npos);
}

TEST_F(CompositeOptimizationTest, MultiplePredicatesSelectivity) {
    // More predicates should reduce selectivity (more restrictive)
    auto single = optimizer.analyze_query(
        "products",
        "category = 'Electronics'",
        10000,
        {}
    );
    
    auto multi = optimizer.analyze_query(
        "products",
        "category = 'Electronics' AND price > 100 AND stock > 0",
        10000,
        {}
    );
    
    EXPECT_GT(single.estimated_selectivity, multi.estimated_selectivity);
}

// ============================================================================
// Statistics Tracking Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, TrackStatistics) {
    // Analyze multiple queries
    optimizer.analyze_query("t1", "id = 1", 10000, {});
    optimizer.analyze_query("t2", "value > 100", 10000, {});
    optimizer.analyze_query("t3", "date BETWEEN '2024-01-01' AND '2024-12-31'", 10000, {});
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.queries_analyzed, 3);
    EXPECT_GT(stats.range_predicates_found, 0);
    EXPECT_GT(stats.indexes_recommended, 0);
}

TEST_F(CompositeOptimizationTest, ResetStatistics) {
    optimizer.analyze_query("t1", "id = 1", 10000, {});
    optimizer.reset_stats();
    
    auto stats = optimizer.get_stats();
    EXPECT_EQ(stats.queries_analyzed, 0);
    EXPECT_EQ(stats.range_predicates_found, 0);
}

// ============================================================================
// Cost Model Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, ScanCostScale) {
    // Scan cost should scale roughly linearly with table size
    double small_cost = optimizer.calculate_scan_cost(1000);
    double large_cost = optimizer.calculate_scan_cost(100000);
    
    EXPECT_GT(large_cost, small_cost);
    // Should be at least 50x more (100000/1000 = 100)
    EXPECT_GT(large_cost / small_cost, 50.0);
}

TEST_F(CompositeOptimizationTest, IndexCostWithSelectivity) {
    // Index cost should increase with selectivity
    double low_sel_cost = optimizer.calculate_index_cost(10000, 0.01);
    double high_sel_cost = optimizer.calculate_index_cost(10000, 0.5);
    
    EXPECT_GT(high_sel_cost, low_sel_cost);
}

TEST_F(CompositeOptimizationTest, IndexBetterThanScan) {
    // For low selectivity, index should be cheaper
    double scan_cost = optimizer.calculate_scan_cost(100000);
    double index_cost = optimizer.calculate_index_cost(100000, 0.01);
    
    EXPECT_LT(index_cost, scan_cost);
}

// ============================================================================
// Decision Quality Tests
// ============================================================================

TEST_F(CompositeOptimizationTest, DecisionIncludesReasoning) {
    auto decision = optimizer.analyze_query(
        "orders",
        "id = 1",
        10000,
        {}
    );
    
    EXPECT_FALSE(decision.reason.empty());
}

TEST_F(CompositeOptimizationTest, ConsistentDecisions) {
    // Same query should produce same decision
    auto d1 = optimizer.analyze_query("orders", "id = 1", 10000, {});
    auto d2 = optimizer.analyze_query("orders", "id = 1", 10000, {});
    
    EXPECT_EQ(d1.use_index, d2.use_index);
    EXPECT_EQ(d1.estimated_selectivity, d2.estimated_selectivity);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
