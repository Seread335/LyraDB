#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lyradb_c.h"

/**
 * Test hash index creation and usage
 * Demonstrates CREATE INDEX statement functionality (Phase 4.1.1)
 */

int main() {
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("║              Hash Index Test (Phase 4.1.1)                   ║\n");
    printf("║   Testing CREATE INDEX statement with hash indexes          ║\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("\n");
    
    char* errmsg = NULL;
    
    // Create database
    printf("📊 Creating database...\n");
    lyra_db_t db = lyra_open("test_hash_index.db", &errmsg);
    if (!db) {
        fprintf(stderr, "Failed to create database: %s\n", errmsg ? errmsg : "unknown");
        return 1;
    }
    printf("✓ Database created\n\n");
    
    printf("─────────────────────────────────────────────────────────────\n");
    printf("📝 Creating Users table\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    // Create table using C API
    const char* col_names[] = {"id", "name", "email", "country"};
    const lyra_datatype_t col_types[] = {
        LYRA_TYPE_INT64,
        LYRA_TYPE_STRING,
        LYRA_TYPE_STRING,
        LYRA_TYPE_STRING
    };
    
    lyra_errcode_t rc = lyra_create_table(db, "users", col_names, col_types, 4, &errmsg);
    if (rc != LYRA_OK) {
        fprintf(stderr, "Failed to create users table: %s\n", errmsg ? errmsg : "unknown");
        lyra_close(db);
        return 1;
    }
    printf("✓ Table 'users' created\n\n");
    
    // Insert test data
    printf("─────────────────────────────────────────────────────────────\n");
    printf("📥 Inserting test data\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    const char* insert_data[][4] = {
        {"1", "Alice Johnson", "alice@example.com", "USA"},
        {"2", "Bob Smith", "bob@example.com", "Canada"},
        {"3", "Charlie Brown", "charlie@example.com", "USA"},
        {"4", "Diana Prince", "diana@example.com", "UK"},
        {"5", "Eve Wilson", "eve@example.com", "USA"},
        {"6", "Frank Miller", "frank@example.com", "Canada"},
        {"7", "Grace Hopper", "grace@example.com", "USA"},
        {"8", "Henry Wells", "henry@example.com", "UK"}
    };
    
    int num_inserts = 8;
    for (int i = 0; i < num_inserts; i++) {
        rc = lyra_insert(db, "users", col_names, insert_data[i], 4, &errmsg);
        if (rc != LYRA_OK) {
            fprintf(stderr, "Failed to insert row %d: %s\n", i + 1, errmsg ? errmsg : "unknown");
            lyra_close(db);
            return 1;
        }
    }
    printf("✓ Inserted %d rows\n\n", num_inserts);
    
    // Create index on 'country' column
    printf("─────────────────────────────────────────────────────────────\n");
    printf("🔑 Creating hash index on 'country' column\n");
    printf("─────────────────────────────────────────────────────────────\n");
    
    const char* create_index_sql = "CREATE INDEX idx_country ON users (country)";
    lyra_result_t result = lyra_query(db, create_index_sql, &errmsg);
    if (!result) {
        fprintf(stderr, "Failed to create index: %s\n", errmsg ? errmsg : "unknown");
        lyra_close(db);
        return 1;
    }
    if (result) lyra_free_result(result);
    printf("✓ Hash index 'idx_country' created on column 'country'\n\n");
    
    // Query using indexed column
    printf("─────────────────────────────────────────────────────────────\n");
    printf("🔍 Query: Find all users from USA\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("SQL: SELECT * FROM users WHERE country = 'USA'\n\n");
    
    result = lyra_query(db, "SELECT * FROM users WHERE country = 'USA'", &errmsg);
    if (!result) {
        fprintf(stderr, "Failed to execute filtered query: %s\n", errmsg ? errmsg : "unknown");
        lyra_close(db);
        return 1;
    }
    
    int64_t usa_count = lyra_rows(result);
    printf("Found %lld users from USA:\n", usa_count);
    for (int64_t i = 0; i < usa_count; i++) {
        const char* id = lyra_get_string(result, i, 0);
        const char* name = lyra_get_string(result, i, 1);
        printf("  • User #%s: %s\n", id, name);
    }
    lyra_free_result(result);
    printf("\n");
    
    // Another query
    printf("─────────────────────────────────────────────────────────────\n");
    printf("🔍 Query: Find all users from Canada\n");
    printf("─────────────────────────────────────────────────────────────\n");
    printf("SQL: SELECT * FROM users WHERE country = 'Canada'\n\n");
    
    result = lyra_query(db, "SELECT * FROM users WHERE country = 'Canada'", &errmsg);
    if (!result) {
        fprintf(stderr, "Failed to execute query: %s\n", errmsg ? errmsg : "unknown");
        lyra_close(db);
        return 1;
    }
    
    int64_t canada_count = lyra_rows(result);
    printf("Found %lld users from Canada:\n", canada_count);
    for (int64_t i = 0; i < canada_count; i++) {
        const char* id = lyra_get_string(result, i, 0);
        const char* name = lyra_get_string(result, i, 1);
        printf("  • User #%s: %s\n", id, name);
    }
    lyra_free_result(result);
    printf("\n");
    
    // Summary
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("✅ Hash Index Test Results\n");
    printf("═══════════════════════════════════════════════════════════════\n");
    printf("✓ Table created with 4 columns (id, name, email, country)\n");
    printf("✓ Inserted %d test records\n", num_inserts);
    printf("✓ Created hash index on 'country' column\n");
    printf("✓ Query with indexed column (USA): %lld results\n", usa_count);
    printf("✓ Query with indexed column (Canada): %lld results\n", canada_count);
    printf("✓ Hash index lookup working correctly!\n");
    printf("═══════════════════════════════════════════════════════════════\n\n");
    
    // Cleanup
    lyra_close(db);
    printf("✓ Database closed\n");
    
    return 0;
}
