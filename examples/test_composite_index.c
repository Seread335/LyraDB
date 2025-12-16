/**
 * @file test_composite_index_simple.c
 * @brief Simple C test for composite hash index functionality
 * 
 * Tests creating and querying composite indexes on multiple columns.
 * Phase 4.1.2 implementation verification.
 */

#include "lyradb_c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    printf("=== LyraDB Composite Hash Index Test (Phase 4.1.2) ===\n\n");
    
    // Create database
    lyradb_handle_t db = lyradb_create();
    if (!db) {
        printf("ERROR: Failed to create database\n");
        return 1;
    }
    printf("✓ Database created\n");
    
    // Create table with multiple columns
    lyradb_schema_t schema = lyradb_schema_create();
    lyradb_schema_add_column(schema, "country", LYRADB_VARCHAR);
    lyradb_schema_add_column(schema, "city", LYRADB_VARCHAR);
    lyradb_schema_add_column(schema, "population", LYRADB_INT);
    
    if (lyradb_create_table(db, "cities", schema) != LYRADB_OK) {
        printf("ERROR: Failed to create cities table\n");
        lyradb_schema_destroy(schema);
        lyradb_close(db);
        return 1;
    }
    printf("✓ Table 'cities' created with columns: country, city, population\n");
    lyradb_schema_destroy(schema);
    
    // Insert test data
    lyradb_insert_row(db, "cities", (const char*[]){"USA", "New York", "8000000"}, 3);
    lyradb_insert_row(db, "cities", (const char*[]){"USA", "Los Angeles", "4000000"}, 3);
    lyradb_insert_row(db, "cities", (const char*[]){"Canada", "Toronto", "2930000"}, 3);
    lyradb_insert_row(db, "cities", (const char*[]){"Canada", "Vancouver", "675000"}, 3);
    lyradb_insert_row(db, "cities", (const char*[]){"USA", "Chicago", "2700000"}, 3);
    printf("✓ Inserted 5 rows of test data\n");
    
    // Create composite index on (country, city)
    if (lyradb_execute_query(db, "CREATE INDEX idx_country_city ON cities (country, city)") != LYRADB_OK) {
        printf("ERROR: Failed to create composite index\n");
        lyradb_close(db);
        return 1;
    }
    printf("✓ Composite index created on columns (country, city)\n");
    
    // Test 1: Lookup with composite key (USA, Los Angeles)
    printf("\nTest 1: Lookup (USA, Los Angeles)\n");
    lyradb_query_result_t result = lyradb_execute_query(db, 
        "SELECT * FROM cities WHERE country='USA' AND city='Los Angeles'");
    
    if (result && lyradb_result_row_count(result) > 0) {
        printf("  ✓ Found %d row(s)\n", lyradb_result_row_count(result));
        printf("  Country: %s, City: %s, Population: %s\n",
            lyradb_result_get_value(result, 0, 0),
            lyradb_result_get_value(result, 0, 1),
            lyradb_result_get_value(result, 0, 2));
    } else {
        printf("  ERROR: Expected 1 row, got %d\n", 
            result ? lyradb_result_row_count(result) : -1);
        if (result) lyradb_result_destroy(result);
        lyradb_close(db);
        return 1;
    }
    lyradb_result_destroy(result);
    
    // Test 2: Lookup with different composite key (Canada, Toronto)
    printf("\nTest 2: Lookup (Canada, Toronto)\n");
    result = lyradb_execute_query(db,
        "SELECT * FROM cities WHERE country='Canada' AND city='Toronto'");
    
    if (result && lyradb_result_row_count(result) > 0) {
        printf("  ✓ Found %d row(s)\n", lyradb_result_row_count(result));
        printf("  Country: %s, City: %s, Population: %s\n",
            lyradb_result_get_value(result, 0, 0),
            lyradb_result_get_value(result, 0, 1),
            lyradb_result_get_value(result, 0, 2));
    } else {
        printf("  ERROR: Expected 1 row, got %d\n",
            result ? lyradb_result_row_count(result) : -1);
        if (result) lyradb_result_destroy(result);
        lyradb_close(db);
        return 1;
    }
    lyradb_result_destroy(result);
    
    // Test 3: Lookup with non-existent composite key
    printf("\nTest 3: Lookup (Brazil, Rio) - should return 0 rows\n");
    result = lyradb_execute_query(db,
        "SELECT * FROM cities WHERE country='Brazil' AND city='Rio'");
    
    if (result && lyradb_result_row_count(result) == 0) {
        printf("  ✓ Correctly returned empty result\n");
    } else {
        printf("  ERROR: Expected 0 rows, got %d\n",
            result ? lyradb_result_row_count(result) : -1);
        if (result) lyradb_result_destroy(result);
        lyradb_close(db);
        return 1;
    }
    lyradb_result_destroy(result);
    
    // Test 4: Create a second composite index on different columns
    printf("\nTest 4: Create second composite index (city, country)\n");
    if (lyradb_execute_query(db, "CREATE INDEX idx_city_country ON cities (city, country)") != LYRADB_OK) {
        printf("  ERROR: Failed to create second composite index\n");
        lyradb_close(db);
        return 1;
    }
    printf("  ✓ Second composite index created\n");
    
    // Cleanup
    lyradb_close(db);
    
    printf("\n=== All Tests Passed! ===\n");
    printf("Phase 4.1.2: Multi-column hash indexes working correctly\n");
    
    return 0;
}
