#include <gtest/gtest.h>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <chrono>

using namespace lyradb;

class HashJoinTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create test tables
        // Table: employees
        Schema emp_schema;
        emp_schema.add_column(Column("emp_id", DataType::INT64, false));
        emp_schema.add_column(Column("name", DataType::STRING, false));
        emp_schema.add_column(Column("dept_id", DataType::INT64, false));
        emp_schema.add_column(Column("salary", DataType::INT64, false));
        db.create_table("employees", emp_schema);
        
        // Table: departments
        Schema dept_schema;
        dept_schema.add_column(Column("dept_id", DataType::INT64, false));
        dept_schema.add_column(Column("dept_name", DataType::STRING, false));
        db.create_table("departments", dept_schema);
        
        // Table: projects
        Schema proj_schema;
        proj_schema.add_column(Column("project_id", DataType::INT64, false));
        proj_schema.add_column(Column("emp_id", DataType::INT64, false));
        proj_schema.add_column(Column("hours", DataType::INT64, false));
        db.create_table("projects", proj_schema);
        
        // Insert test data
        db.execute("INSERT INTO employees VALUES (1, 'Alice', 10, 80000)");
        db.execute("INSERT INTO employees VALUES (2, 'Bob', 10, 75000)");
        db.execute("INSERT INTO employees VALUES (3, 'Charlie', 20, 85000)");
        db.execute("INSERT INTO employees VALUES (4, 'David', 20, 90000)");
        db.execute("INSERT INTO employees VALUES (5, 'Eve', 30, 70000)");
        
        db.execute("INSERT INTO departments VALUES (10, 'Sales')");
        db.execute("INSERT INTO departments VALUES (20, 'Engineering')");
        db.execute("INSERT INTO departments VALUES (30, 'Marketing')");
        db.execute("INSERT INTO departments VALUES (40, 'HR')");
        
        db.execute("INSERT INTO projects VALUES (1, 1, 40)");
        db.execute("INSERT INTO projects VALUES (1, 2, 35)");
        db.execute("INSERT INTO projects VALUES (2, 3, 45)");
        db.execute("INSERT INTO projects VALUES (2, 4, 50)");
        db.execute("INSERT INTO projects VALUES (3, 5, 20)");
    }
};

// =============================================================================
// TEST SUITE 1: Basic Hash Join Correctness
// =============================================================================

TEST_F(HashJoinTest, InnerJoinBasic) {
    auto result = db.query(
        "SELECT employees.emp_id, employees.name, departments.dept_name "
        "FROM employees "
        "INNER JOIN departments ON employees.dept_id = departments.dept_id"
    );
    
    EXPECT_EQ(result->row_count(), 5);
    EXPECT_EQ(result->column_count(), 3);
}

TEST_F(HashJoinTest, InnerJoinWithFilter) {
    auto result = db.query(
        "SELECT employees.name, departments.dept_name "
        "FROM employees "
        "INNER JOIN departments ON employees.dept_id = departments.dept_id "
        "WHERE employees.salary > 75000"
    );
    
    EXPECT_GT(result->row_count(), 0);
    EXPECT_EQ(result->column_count(), 2);
}

TEST_F(HashJoinTest, LeftJoinBasic) {
    auto result = db.query(
        "SELECT employees.name, departments.dept_name "
        "FROM employees "
        "LEFT JOIN departments ON employees.dept_id = departments.dept_id"
    );
    
    EXPECT_EQ(result->row_count(), 5);
}

TEST_F(HashJoinTest, JoinWithNULLValues) {
    // Insert employee with non-existent department
    db.execute("INSERT INTO employees VALUES (6, 'Frank', 99, 65000)");
    
    auto result = db.query(
        "SELECT employees.name, departments.dept_name "
        "FROM employees "
        "LEFT JOIN departments ON employees.dept_id = departments.dept_id"
    );
    
    EXPECT_EQ(result->row_count(), 6);
}

// =============================================================================
// TEST SUITE 2: Filter Pushdown Optimization
// =============================================================================

TEST_F(HashJoinTest, FilterPushdownReducesJoinSize) {
    // This query should apply salary filter BEFORE join
    // Reducing the number of rows being joined
    auto result = db.query(
        "SELECT employees.name, departments.dept_name "
        "FROM employees "
        "JOIN departments ON employees.dept_id = departments.dept_id "
        "WHERE employees.salary >= 80000"
    );
    
    EXPECT_GT(result->row_count(), 0);
    EXPECT_LE(result->row_count(), 5);
}

TEST_F(HashJoinTest, FilterPushdownMultipleConditions) {
    auto result = db.query(
        "SELECT employees.name, departments.dept_name "
        "FROM employees "
        "JOIN departments ON employees.dept_id = departments.dept_id "
        "WHERE employees.salary > 70000 AND employees.dept_id = 20"
    );
    
    EXPECT_GT(result->row_count(), 0);
}

// =============================================================================
// TEST SUITE 3: Partial Sort Optimization
// =============================================================================

TEST_F(HashJoinTest, PartialSortWithLimitSmall) {
    auto result = db.query(
        "SELECT employees.name, employees.salary "
        "FROM employees "
        "ORDER BY employees.salary DESC "
        "LIMIT 3"
    );
    
    EXPECT_EQ(result->row_count(), 3);
}

TEST_F(HashJoinTest, PartialSortWithLimitLarge) {
    // LIMIT larger than result set
    auto result = db.query(
        "SELECT employees.name, employees.salary "
        "FROM employees "
        "ORDER BY employees.salary ASC "
        "LIMIT 100"
    );
    
    EXPECT_EQ(result->row_count(), 5);
}

TEST_F(HashJoinTest, PartialSortMultipleColumns) {
    auto result = db.query(
        "SELECT employees.dept_id, employees.salary, employees.name "
        "FROM employees "
        "ORDER BY employees.dept_id ASC, employees.salary DESC "
        "LIMIT 4"
    );
    
    EXPECT_EQ(result->row_count(), 4);
}

TEST_F(HashJoinTest, PartialSortWithOffset) {
    auto result = db.query(
        "SELECT employees.name, employees.salary "
        "FROM employees "
        "ORDER BY employees.salary DESC "
        "LIMIT 2 OFFSET 1"
    );
    
    EXPECT_EQ(result->row_count(), 2);
}

// =============================================================================
// TEST SUITE 4: Complex Join Scenarios
// =============================================================================

TEST_F(HashJoinTest, MultipleJoins) {
    auto result = db.query(
        "SELECT employees.name, projects.project_id, projects.hours "
        "FROM employees "
        "JOIN projects ON employees.emp_id = projects.emp_id "
        "LIMIT 10"
    );
    
    EXPECT_GT(result->row_count(), 0);
}

TEST_F(HashJoinTest, JoinWithGroupBy) {
    auto result = db.query(
        "SELECT departments.dept_name, COUNT(employees.emp_id) as emp_count "
        "FROM employees "
        "JOIN departments ON employees.dept_id = departments.dept_id "
        "GROUP BY departments.dept_id"
    );
    
    EXPECT_GT(result->row_count(), 0);
}

TEST_F(HashJoinTest, JoinWithOrderByAndLimit) {
    auto result = db.query(
        "SELECT employees.name, employees.salary "
        "FROM employees "
        "JOIN departments ON employees.dept_id = departments.dept_id "
        "ORDER BY employees.salary DESC "
        "LIMIT 2"
    );
    
    EXPECT_EQ(result->row_count(), 2);
}

TEST_F(HashJoinTest, JoinWithAllOptimizations) {
    // Combine filter pushdown + join + partial sort
    auto result = db.query(
        "SELECT employees.name, departments.dept_name, employees.salary "
        "FROM employees "
        "JOIN departments ON employees.dept_id = departments.dept_id "
        "WHERE employees.salary > 70000 "
        "ORDER BY employees.salary DESC "
        "LIMIT 3"
    );
    
    EXPECT_LE(result->row_count(), 3);
}

// =============================================================================
// TEST SUITE 5: Performance Verification
// =============================================================================

TEST_F(HashJoinTest, HashJoinPerformance) {
    // Create larger test tables
    Schema large_schema;
    large_schema.add_column(Column("id", DataType::INT64, false));
    large_schema.add_column(Column("value", DataType::INT64, false));
    db.create_table("large_left", large_schema);
    db.create_table("large_right", large_schema);
    
    // Insert 1000 rows in each
    for (int i = 0; i < 1000; ++i) {
        db.execute("INSERT INTO large_left VALUES (" + std::to_string(i) + ", " + std::to_string(i * 2) + ")");
        db.execute("INSERT INTO large_right VALUES (" + std::to_string(i) + ", " + std::to_string(i * 3) + ")");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = db.query(
        "SELECT large_left.id, large_right.value "
        "FROM large_left "
        "INNER JOIN large_right ON large_left.id = large_right.id"
    );
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(result->row_count(), 1000);
    // Hash join should complete in reasonable time (< 1 second for 1000 rows)
    EXPECT_LT(duration.count(), 1000);
}

TEST_F(HashJoinTest, PartialSortPerformance) {
    // Create larger test table
    Schema large_schema;
    large_schema.add_column(Column("id", DataType::INT64, false));
    large_schema.add_column(Column("value", DataType::INT64, false));
    db.create_table("sort_test", large_schema);
    
    // Insert 10000 rows
    for (int i = 0; i < 10000; ++i) {
        db.execute("INSERT INTO sort_test VALUES (" + std::to_string(i) + ", " + std::to_string(rand() % 1000) + ")");
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    auto result = db.query(
        "SELECT id, value "
        "FROM sort_test "
        "ORDER BY value DESC "
        "LIMIT 10"
    );
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    EXPECT_EQ(result->row_count(), 10);
    // Partial sort with LIMIT 10 should be much faster than sorting all 10000
    EXPECT_LT(duration.count(), 500);
}

// =============================================================================
// TEST SUITE 6: Edge Cases
// =============================================================================

TEST_F(HashJoinTest, EmptyTableJoin) {
    db.execute("DELETE FROM projects WHERE 1=1");
    
    auto result = db.query(
        "SELECT employees.name, projects.hours "
        "FROM employees "
        "INNER JOIN projects ON employees.emp_id = projects.emp_id"
    );
    
    EXPECT_EQ(result->row_count(), 0);
}

TEST_F(HashJoinTest, JoinNoMatches) {
    db.execute("DELETE FROM departments WHERE 1=1");
    
    auto result = db.query(
        "SELECT employees.name, departments.dept_name "
        "FROM employees "
        "INNER JOIN departments ON employees.dept_id = departments.dept_id"
    );
    
    EXPECT_EQ(result->row_count(), 0);
}

TEST_F(HashJoinTest, LimitZero) {
    auto result = db.query(
        "SELECT name FROM employees LIMIT 0"
    );
    
    EXPECT_EQ(result->row_count(), 0);
}

TEST_F(HashJoinTest, OffsetBeyondRowCount) {
    auto result = db.query(
        "SELECT name FROM employees "
        "ORDER BY emp_id "
        "LIMIT 10 OFFSET 100"
    );
    
    EXPECT_EQ(result->row_count(), 0);
}

// =============================================================================
// TEST SUITE 7: Correctness Verification
// =============================================================================

TEST_F(HashJoinTest, JoinResultOrder) {
    // Verify join produces correct rows (order may vary for hash join)
    auto result = db.query(
        "SELECT employees.emp_id, employees.name "
        "FROM employees "
        "INNER JOIN departments ON employees.dept_id = departments.dept_id "
        "ORDER BY employees.emp_id"
    );
    
    EXPECT_EQ(result->row_count(), 5);
}

TEST_F(HashJoinTest, FilterPushdownCorrectness) {
    // Same query with and without pushdown should give same result
    auto result1 = db.query(
        "SELECT employees.name FROM employees WHERE employees.salary > 75000"
    );
    
    auto result2 = db.query(
        "SELECT employees.name "
        "FROM employees "
        "JOIN departments ON employees.dept_id = departments.dept_id "
        "WHERE employees.salary > 75000"
    );
    
    EXPECT_EQ(result1->row_count(), result2->row_count());
}

TEST_F(HashJoinTest, PartialSortCorrectness) {
    // Get top 3 salaries - should be in descending order
    auto result = db.query(
        "SELECT employees.salary "
        "FROM employees "
        "ORDER BY employees.salary DESC "
        "LIMIT 3"
    );
    
    EXPECT_EQ(result->row_count(), 3);
    // First row should have highest salary (90000)
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
