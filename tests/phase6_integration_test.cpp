#include <iostream>
#include <vector>
#include <memory>
#include <string>
#include "lyradb/phase44_optimizer.h"

using namespace lyradb::integration;

/**
 * Phase 6 Integration Test
 * Demonstrates real-world usage of Phase 4.4 optimization modules
 */

void test_single_predicate_optimization() {
    std::cout << "\n=== Test 1: Single Predicate Optimization ===\n";
    
    Phase44QueryOptimizer optimizer;
    
    // Register index
    optimizer.register_index("age_idx", "age", 100);
    
    // Test highly selective predicate
    auto plan = optimizer.optimize_where_clause("age = 25", 1000000, {"age_idx"});
    
    std::cout << "Query: " << plan.query_text << "\n";
    std::cout << "Strategy: " << plan.strategy << "\n";
    std::cout << "Predicted Speedup: " << plan.predicted_speedup << "x\n";
    std::cout << "Notes: " << plan.execution_notes << "\n";
    
    // Test low selectivity predicate
    plan = optimizer.optimize_where_clause("age > 18", 1000000, {"age_idx"});
    
    std::cout << "\nQuery: " << plan.query_text << "\n";
    std::cout << "Strategy: " << plan.strategy << "\n";
    std::cout << "Predicted Speedup: " << plan.predicted_speedup << "x\n";
    std::cout << "Notes: " << plan.execution_notes << "\n";
}

void test_composite_predicate_optimization() {
    std::cout << "\n=== Test 2: Composite Predicate Optimization ===\n";
    
    Phase44QueryOptimizer optimizer;
    
    // Register indexes
    optimizer.register_index("age_idx", "age", 100);
    optimizer.register_index("country_idx", "country", 250);
    
    // Test AND predicate
    auto plan = optimizer.optimize_where_clause(
        "age = 25 AND country = USA",
        1000000,
        {"age_idx", "country_idx"});
    
    std::cout << "Query: " << plan.query_text << "\n";
    std::cout << "Strategy: " << plan.strategy << "\n";
    std::cout << "Predicted Speedup: " << plan.predicted_speedup << "x\n";
    std::cout << "Indexes Used: ";
    for (const auto& idx : plan.indexes_used) {
        std::cout << idx << " ";
    }
    std::cout << "\n";
    std::cout << "Notes: " << plan.execution_notes << "\n";
}

void test_or_predicate_optimization() {
    std::cout << "\n=== Test 3: OR Predicate Optimization ===\n";
    
    Phase44QueryOptimizer optimizer;
    
    // Register indexes
    optimizer.register_index("status_idx", "status", 5);
    
    // Test OR predicate
    auto plan = optimizer.optimize_where_clause(
        "status = active OR status = pending",
        1000000,
        {"status_idx"});
    
    std::cout << "Query: " << plan.query_text << "\n";
    std::cout << "Strategy: " << plan.strategy << "\n";
    std::cout << "Predicted Speedup: " << plan.predicted_speedup << "x\n";
    std::cout << "Notes: " << plan.execution_notes << "\n";
}

void test_learning_from_execution() {
    std::cout << "\n=== Test 4: Learning from Actual Execution ===\n";
    
    Phase44QueryOptimizer optimizer;
    
    // Simulate execution results
    std::cout << "Recording execution result:\n";
    std::cout << "  Query: age = 25\n";
    std::cout << "  Strategy: index_single\n";
    std::cout << "  Rows examined: 1000\n";
    std::cout << "  Rows matched: 5000\n";
    std::cout << "  Execution time: 5.2ms\n";
    
    optimizer.record_execution_result(
        "age = 25",
        "index_single",
        1000,
        5000,
        5.2);
    
    // Query 2
    std::cout << "\nRecording execution result:\n";
    std::cout << "  Query: age > 18 AND country = USA\n";
    std::cout << "  Strategy: index_intersection\n";
    std::cout << "  Rows examined: 500000\n";
    std::cout << "  Rows matched: 450000\n";
    std::cout << "  Execution time: 245.8ms\n";
    
    optimizer.record_execution_result(
        "age > 18 AND country = USA",
        "index_intersection",
        500000,
        450000,
        245.8);
}

void test_real_world_scenario() {
    std::cout << "\n=== Test 5: E-Commerce Real-World Scenario ===\n";
    
    Phase44QueryOptimizer optimizer;
    
    // Register indexes for e-commerce database
    optimizer.register_index("customer_id_idx", "customer_id", 100000);
    optimizer.register_index("order_status_idx", "order_status", 5);
    optimizer.register_index("created_date_idx", "created_date", 365);
    optimizer.register_index("total_price_idx", "total_price", 10000);
    
    // Optimize realistic e-commerce queries
    std::vector<std::pair<std::string, std::string>> test_queries = {
        {"customer_id = 12345", "Lookup specific customer order"},
        {"order_status = shipped", "Find all shipped orders"},
        {"created_date > 2024-01-01 AND order_status = completed", "Recent completed orders"},
    };
    
    for (const auto& [query, description] : test_queries) {
        std::cout << "\nDescription: " << description << "\n";
        
        auto plan = optimizer.optimize_where_clause(
            query,
            10000000,
            {"customer_id_idx", "order_status_idx", "created_date_idx", "total_price_idx"});
        
        std::cout << "Query: " << plan.query_text << "\n";
        std::cout << "Strategy: " << plan.strategy << "\n";
        std::cout << "Predicted Speedup: " << plan.predicted_speedup << "x\n";
    }
}

void test_statistics() {
    std::cout << "\n=== Test 6: Optimization Statistics ===\n";
    
    Phase44QueryOptimizer optimizer;
    
    // Perform several optimizations
    optimizer.optimize_where_clause("age = 25", 1000000, {"age_idx"});
    optimizer.optimize_where_clause("status = active", 1000000, {"status_idx"});
    optimizer.optimize_where_clause("age > 18 AND country = USA", 1000000, {"age_idx", "country_idx"});
    
    // Print statistics
    std::cout << optimizer.get_optimization_stats();
}

int main() {
    std::cout << "╔════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║   Phase 6: Real-World Integration Test Suite                   ║\n";
    std::cout << "║   LyraDB Query Optimization (Phase 4.4 Modules)                ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
    
    try {
        test_single_predicate_optimization();
        test_composite_predicate_optimization();
        test_or_predicate_optimization();
        test_learning_from_execution();
        test_real_world_scenario();
        test_statistics();
        
        std::cout << "\n╔════════════════════════════════════════════════════════════════╗\n";
        std::cout << "║   ✓ All Phase 6 Integration Tests Completed Successfully        ║\n";
        std::cout << "╚════════════════════════════════════════════════════════════════╝\n";
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n✗ Error: " << e.what() << "\n";
        return 1;
    }
}
