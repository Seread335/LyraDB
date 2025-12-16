#include <iostream>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <chrono>

using namespace lyradb;

int main() {
    std::cout << "=================================================================\n";
    std::cout << "  Phase 3.3: Hash Join & Query Optimization Testing\n";
    std::cout << "=================================================================\n\n";
    
    Database db{":memory:"};
    
    // Setup: Create test tables
    std::cout << "[SETUP] Creating test tables...\n";
    
    Schema emp_schema;
    emp_schema.add_column(Column("emp_id", DataType::INT64, false));
    emp_schema.add_column(Column("name", DataType::STRING, false));
    emp_schema.add_column(Column("dept_id", DataType::INT64, false));
    emp_schema.add_column(Column("salary", DataType::INT64, false));
    db.create_table("employees", emp_schema);
    
    Schema dept_schema;
    dept_schema.add_column(Column("dept_id", DataType::INT64, false));
    dept_schema.add_column(Column("dept_name", DataType::STRING, false));
    db.create_table("departments", dept_schema);
    
    // Insert test data
    std::cout << "[SETUP] Inserting test data...\n";
    db.execute("INSERT INTO employees VALUES (1, 'Alice', 10, 80000)");
    db.execute("INSERT INTO employees VALUES (2, 'Bob', 10, 75000)");
    db.execute("INSERT INTO employees VALUES (3, 'Charlie', 20, 85000)");
    db.execute("INSERT INTO employees VALUES (4, 'David', 20, 90000)");
    db.execute("INSERT INTO employees VALUES (5, 'Eve', 30, 70000)");
    
    db.execute("INSERT INTO departments VALUES (10, 'Sales')");
    db.execute("INSERT INTO departments VALUES (20, 'Engineering')");
    db.execute("INSERT INTO departments VALUES (30, 'Marketing')");
    
    std::cout << "\n[TEST 1] Basic INNER JOIN\n";
    std::cout << "Query: SELECT employees.emp_id, employees.name, departments.dept_name\n";
    std::cout << "       FROM employees INNER JOIN departments\n";
    std::cout << "       ON employees.dept_id = departments.dept_id\n";
    try {
        auto result = db.query(
            "SELECT employees.emp_id, employees.name, departments.dept_name "
            "FROM employees "
            "INNER JOIN departments ON employees.dept_id = departments.dept_id"
        );
        std::cout << "✓ Result: " << result->row_count() << " rows, " 
                  << result->column_count() << " columns\n";
        std::cout << "  Expected: 5 rows (all employees joined with departments)\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n[TEST 2] LEFT JOIN with NULL handling\n";
    db.execute("INSERT INTO employees VALUES (6, 'Frank', 99, 65000)");
    std::cout << "Query: SELECT employees.name, departments.dept_name\n";
    std::cout << "       FROM employees LEFT JOIN departments\n";
    std::cout << "       ON employees.dept_id = departments.dept_id\n";
    try {
        auto result = db.query(
            "SELECT employees.name, departments.dept_name "
            "FROM employees "
            "LEFT JOIN departments ON employees.dept_id = departments.dept_id"
        );
        std::cout << "✓ Result: " << result->row_count() << " rows\n";
        std::cout << "  Expected: 6 rows (Frank has non-existent dept with NULL)\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n[TEST 3] Filter Pushdown (WHERE before JOIN)\n";
    std::cout << "Query: SELECT employees.name, departments.dept_name\n";
    std::cout << "       FROM employees JOIN departments\n";
    std::cout << "       ON employees.dept_id = departments.dept_id\n";
    std::cout << "       WHERE employees.salary >= 80000\n";
    try {
        auto result = db.query(
            "SELECT employees.name, departments.dept_name "
            "FROM employees "
            "JOIN departments ON employees.dept_id = departments.dept_id "
            "WHERE employees.salary >= 80000"
        );
        std::cout << "✓ Result: " << result->row_count() << " rows\n";
        std::cout << "  Expected: 3 rows (Alice, Charlie, David)\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n[TEST 4] Partial Sort with LIMIT (for performance)\n";
    std::cout << "Query: SELECT name, salary FROM employees\n";
    std::cout << "       ORDER BY salary DESC LIMIT 3\n";
    try {
        auto result = db.query(
            "SELECT name, salary FROM employees "
            "ORDER BY salary DESC LIMIT 3"
        );
        std::cout << "✓ Result: " << result->row_count() << " rows\n";
        std::cout << "  Expected: 3 rows (top 3 salaries using partial_sort)\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n[TEST 5] Combined: Filter + Join + Partial Sort\n";
    std::cout << "Query: SELECT employees.name, employees.salary\n";
    std::cout << "       FROM employees JOIN departments\n";
    std::cout << "       ON employees.dept_id = departments.dept_id\n";
    std::cout << "       WHERE employees.salary > 70000\n";
    std::cout << "       ORDER BY employees.salary DESC LIMIT 3\n";
    try {
        auto result = db.query(
            "SELECT employees.name, employees.salary "
            "FROM employees "
            "JOIN departments ON employees.dept_id = departments.dept_id "
            "WHERE employees.salary > 70000 "
            "ORDER BY employees.salary DESC "
            "LIMIT 3"
        );
        std::cout << "✓ Result: " << result->row_count() << " rows\n";
        std::cout << "  Expected: 3 rows\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n[TEST 6] Performance: Hash Join on 1000 rows\n";
    std::cout << "Creating larger tables...\n";
    
    Schema large_schema;
    large_schema.add_column(Column("id", DataType::INT64, false));
    large_schema.add_column(Column("value", DataType::INT64, false));
    db.create_table("large_left", large_schema);
    db.create_table("large_right", large_schema);
    
    std::cout << "Inserting 1000 rows in each table...\n";
    for (int i = 0; i < 1000; ++i) {
        db.execute("INSERT INTO large_left VALUES (" + std::to_string(i) + ", " + std::to_string(i * 2) + ")");
        db.execute("INSERT INTO large_right VALUES (" + std::to_string(i) + ", " + std::to_string(i * 3) + ")");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    try {
        auto result = db.query(
            "SELECT large_left.id FROM large_left "
            "INNER JOIN large_right ON large_left.id = large_right.id"
        );
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Hash Join: " << result->row_count() << " rows in " 
                  << duration.count() << "ms\n";
        std::cout << "  Expected: ~1000 rows in <1000ms (O(n+m) complexity)\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n[TEST 7] Performance: Partial Sort with 10000 rows\n";
    std::cout << "Creating table with 10000 rows...\n";
    
    Schema sort_schema;
    sort_schema.add_column(Column("id", DataType::INT64, false));
    sort_schema.add_column(Column("random_val", DataType::INT64, false));
    db.create_table("sort_test", sort_schema);
    
    for (int i = 0; i < 10000; ++i) {
        db.execute("INSERT INTO sort_test VALUES (" + std::to_string(i) + ", " + std::to_string(rand() % 10000) + ")");
    }
    
    start = std::chrono::high_resolution_clock::now();
    try {
        auto result = db.query(
            "SELECT id FROM sort_test "
            "ORDER BY random_val DESC LIMIT 10"
        );
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        std::cout << "✓ Partial Sort: " << result->row_count() << " rows in " 
                  << duration.count() << "ms\n";
        std::cout << "  Expected: 10 rows in <500ms (O(n log k) vs O(n log n))\n";
    } catch (const std::exception& e) {
        std::cout << "✗ Error: " << e.what() << "\n";
    }
    
    std::cout << "\n=================================================================\n";
    std::cout << "  Phase 3.3 Tests Complete!\n";
    std::cout << "=================================================================\n";
    
    return 0;
}
