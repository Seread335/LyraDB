#include <stdio.h>
#include <stdlib.h>
#include "../include/lyradb_c.h"

int main(int argc, char* argv[]) {
    printf("╔════════════════════════════════════════════════════════════════╗\n");
    printf("║          LyraDB Embedded Library - Simple Example              ║\n");
    printf("╚════════════════════════════════════════════════════════════════╝\n\n");

    // Open database
    char* errmsg = NULL;
    printf("📂 Opening database...\n");
    lyra_db_t db = lyra_open("example.db", &errmsg);
    if (!db) {
        printf("❌ Error opening database: %s\n", errmsg);
        free(errmsg);
        return 1;
    }
    printf("✅ Database opened successfully\n\n");

    // Create table
    printf("📋 Creating table...\n");
    const char* col_names[] = {"id", "name", "age"};
    const lyra_datatype_t col_types[] = {LYRA_TYPE_INT64, LYRA_TYPE_STRING, LYRA_TYPE_INT32};
    
    lyra_errcode_t rc = lyra_create_table(db, "users", col_names, col_types, 3, &errmsg);
    if (rc != LYRA_OK) {
        printf("⚠️  Note: Table may already exist\n");
    }
    printf("✅ Table 'users' ready\n\n");

    // Insert data
    printf("📝 Inserting sample data...\n");
    const char* insert_names[] = {"id", "name", "age"};
    
    const char* values1[] = {"1", "Alice", "30"};
    lyra_insert(db, "users", insert_names, values1, 3, &errmsg);
    
    const char* values2[] = {"2", "Bob", "25"};
    lyra_insert(db, "users", insert_names, values2, 3, &errmsg);
    
    const char* values3[] = {"3", "Charlie", "35"};
    lyra_insert(db, "users", insert_names, values3, 3, &errmsg);
    
    printf("✅ Inserted 3 rows\n\n");

    // Query data
    printf("🔍 Executing query: SELECT * FROM users\n");
    printf("─────────────────────────────────────────────\n");
    
    lyra_result_t result = lyra_query(db, "SELECT * FROM users", &errmsg);
    if (!result) {
        printf("❌ Query error: %s\n", errmsg);
        free(errmsg);
        lyra_close(db);
        return 1;
    }

    // Display results
    int64_t rows = lyra_rows(result);
    int cols = lyra_columns(result);
    
    printf("Results: %lld rows, %d columns\n\n", rows, cols);
    
    if (rows > 0) {
        // Print header
        for (int j = 0; j < cols; j++) {
            printf("%-20s ", lyra_column_name(result, j));
        }
        printf("\n");
        printf("────────────────────────────────────────────────────────\n");
        
        // Print data
        for (int64_t i = 0; i < rows; i++) {
            for (int j = 0; j < cols; j++) {
                if (lyra_is_null(result, i, j)) {
                    printf("%-20s ", "(null)");
                } else {
                    const char* val = lyra_get_string(result, i, j);
                    printf("%-20s ", val);
                }
            }
            printf("\n");
        }
    } else {
        printf("(No data)\n");
    }
    
    printf("─────────────────────────────────────────────\n\n");

    // Prepared statement example
    printf("🔗 Using prepared statement...\n");
    lyra_stmt_t stmt = lyra_prepare(db, "SELECT * FROM users WHERE age > ?1", &errmsg);
    if (stmt) {
        lyra_bind_int(stmt, 1, 28);
        lyra_result_t stmt_result = lyra_execute(stmt, &errmsg);
        if (stmt_result) {
            printf("✅ Found %lld users with age > 28\n", lyra_rows(stmt_result));
            lyra_free_result(stmt_result);
        }
        lyra_free_stmt(stmt);
    }
    printf("\n");

    // Export formats
    printf("📤 Exporting data as JSON...\n");
    const char* json = lyra_result_json(result);
    printf("%s\n\n", json);

    printf("📤 Exporting data as CSV...\n");
    const char* csv = lyra_result_csv(result);
    printf("%s\n\n", csv);

    // Cleanup
    printf("🧹 Cleaning up...\n");
    lyra_free_result(result);
    lyra_close(db);
    
    printf("✅ Complete!\n\n");
    printf("Version: %s\n", lyra_version());
    printf("Build: %s\n", lyra_build_info());

    return 0;
}
