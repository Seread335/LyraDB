/**
 * @file btree_range_query_example.c
 * @brief Example demonstrating B-tree range queries (Phase 4.2)
 * 
 * Shows how to:
 * 1. Create indexes on columns
 * 2. Perform exact-match lookups
 * 3. Perform range queries using B-tree
 * 4. Compare performance with and without indexes
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "lyradb_c.h"

// Helper to display results
void print_results(lyradb_result_t* result) {
    if (!result || result->row_count == 0) {
        printf("No results\n");
        return;
    }
    
    for (size_t i = 0; i < result->row_count; ++i) {
        printf("Row %zu: ", i + 1);
        if (result->columns && result->column_count > 0) {
            printf("(");
            for (size_t j = 0; j < result->column_count; ++j) {
                if (j > 0) printf(", ");
                // Print column value (simplified)
                printf("val");
            }
            printf(")\n");
        }
    }
}

// Measure time for a query
double measure_query_time(lyradb_t* db, const char* query) {
    clock_t start = clock();
    lyradb_execute_sql(db, query);
    clock_t end = clock();
    return (double)(end - start) / CLOCKS_PER_SEC * 1000;  // Return in milliseconds
}

int main() {
    printf("╔═══════════════════════════════════════════════════════════╗\n");
    printf("║     B-Tree Range Query Example (Phase 4.2)                ║\n");
    printf("║  Testing range queries: SELECT WHERE id > x AND id < y    ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n\n");
    
    // Create database
    lyradb_t* db = lyradb_create_database("range_query_demo.db", 1024 * 1024);
    if (!db) {
        printf("Failed to create database\n");
        return 1;
    }
    
    printf("✓ Database created\n");
    
    // Create table
    const char* create_table = R"(
        CREATE TABLE products (
            id INTEGER PRIMARY KEY,
            name VARCHAR(100),
            price REAL,
            stock INTEGER
        )
    )";
    
    int ret = lyradb_execute_sql(db, create_table);
    if (ret != 0) {
        printf("Failed to create table: %d\n", ret);
        lyradb_close_database(db);
        return 1;
    }
    printf("✓ Table 'products' created\n");
    
    // Insert sample data (products with various prices)
    printf("\nInserting 100 product records...\n");
    for (int i = 1; i <= 100; ++i) {
        char insert_sql[256];
        snprintf(insert_sql, sizeof(insert_sql),
            "INSERT INTO products (id, name, price, stock) "
            "VALUES (%d, 'Product_%d', %.2f, %d)",
            i, i, 10.0 + (i * 0.5), 100 - i);
        
        lyradb_execute_sql(db, insert_sql);
    }
    printf("✓ Inserted 100 products\n");
    
    // Example 1: Simple range query
    printf("\n--- Example 1: Range Query (id BETWEEN 25 AND 75) ---\n");
    const char* range_query1 = "SELECT id, name, price FROM products WHERE id >= 25 AND id <= 75";
    printf("Query: %s\n", range_query1);
    double time1 = measure_query_time(db, range_query1);
    printf("Execution time: %.2f ms\n", time1);
    
    // Example 2: Open-ended range query
    printf("\n--- Example 2: Open-ended Range (price > 50.0) ---\n");
    const char* range_query2 = "SELECT id, name, price FROM products WHERE price > 50.0";
    printf("Query: %s\n", range_query2);
    double time2 = measure_query_time(db, range_query2);
    printf("Execution time: %.2f ms\n", time2);
    
    // Example 3: Create index for performance
    printf("\n--- Example 3: Creating Index for Optimization ---\n");
    ret = lyradb_execute_sql(db, "CREATE INDEX idx_product_id ON products(id)");
    if (ret == 0) {
        printf("✓ Index 'idx_product_id' created\n");
        printf("  (Creates both HASH index for = and B-TREE index for range queries)\n");
    } else {
        printf("⚠ Failed to create index: %d\n", ret);
    }
    
    // Example 4: Range query with index
    printf("\n--- Example 4: Same Range Query with Index ---\n");
    printf("Query: %s\n", range_query1);
    double time4 = measure_query_time(db, range_query1);
    printf("Execution time: %.2f ms\n", time4);
    printf("Performance improvement: %.1f%%\n", 
        (time1 > time4) ? ((time1 - time4) / time1 * 100) : 0);
    
    // Example 5: Complex range query
    printf("\n--- Example 5: Complex Range (25 < price < 60) ---\n");
    const char* range_query5 = "SELECT id, name, price FROM products WHERE price > 25.0 AND price < 60.0";
    printf("Query: %s\n", range_query5);
    double time5 = measure_query_time(db, range_query5);
    printf("Execution time: %.2f ms\n", time5);
    
    // Example 6: Multiple indexes on different columns
    printf("\n--- Example 6: Multi-Column Index ---\n");
    ret = lyradb_execute_sql(db, "CREATE INDEX idx_price_stock ON products(price, stock)");
    if (ret == 0) {
        printf("✓ Composite index 'idx_price_stock' created\n");
        printf("  (Can optimize queries using both columns)\n");
    }
    
    // Cleanup
    printf("\n--- Cleanup ---\n");
    ret = lyradb_execute_sql(db, "DROP TABLE products");
    if (ret == 0) {
        printf("✓ Table dropped\n");
    }
    
    lyradb_close_database(db);
    printf("✓ Database closed\n");
    
    printf("\n╔═══════════════════════════════════════════════════════════╗\n");
    printf("║             Example completed successfully!                ║\n");
    printf("║         B-Tree indexes enabled for range queries           ║\n");
    printf("╚═══════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
