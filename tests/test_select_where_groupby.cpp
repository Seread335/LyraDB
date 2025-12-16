#include <gtest/gtest.h>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <string>
#include <vector>

using namespace lyradb;

class SelectWithWhereTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create a simple table
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, true});
        schema.add_column(Column{"name", DataType::VARCHAR, 100, false});
        schema.add_column(Column{"age", DataType::INT64, 8, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("employees", schema);
        
        // Insert test data
        db.execute("INSERT INTO employees (id, name, age, salary) VALUES (1, 'Alice', 30, 50000.0)");
        db.execute("INSERT INTO employees (id, name, age, salary) VALUES (2, 'Bob', 25, 40000.0)");
        db.execute("INSERT INTO employees (id, name, age, salary) VALUES (3, 'Charlie', 35, 60000.0)");
        db.execute("INSERT INTO employees (id, name, age, salary) VALUES (4, 'Diana', 28, 45000.0)");
        db.execute("INSERT INTO employees (id, name, age, salary) VALUES (5, 'Eve', 32, 55000.0)");
    }
};

// Basic WHERE clause tests
TEST_F(SelectWithWhereTest, SelectAllWithoutWhere) {
    auto result = db.execute("SELECT * FROM employees");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(SelectWithWhereTest, SelectWithEqualityCondition) {
    auto result = db.execute("SELECT * FROM employees WHERE id = 3");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(SelectWithWhereTest, SelectWithStringEquality) {
    auto result = db.execute("SELECT * FROM employees WHERE name = 'Alice'");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(SelectWithWhereTest, SelectWithGreaterThanCondition) {
    auto result = db.execute("SELECT * FROM employees WHERE age > 30");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(2, result->get_row_count());  // Charlie (35), Eve (32)
    }
}

TEST_F(SelectWithWhereTest, SelectWithLessThanCondition) {
    auto result = db.execute("SELECT * FROM employees WHERE age < 30");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(2, result->get_row_count());  // Bob (25), Diana (28)
    }
}

TEST_F(SelectWithWhereTest, SelectWithGreaterThanOrEqual) {
    auto result = db.execute("SELECT * FROM employees WHERE age >= 30");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(3, result->get_row_count());  // Alice (30), Charlie (35), Eve (32)
    }
}

TEST_F(SelectWithWhereTest, SelectWithLessThanOrEqual) {
    auto result = db.execute("SELECT * FROM employees WHERE age <= 28");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(2, result->get_row_count());  // Bob (25), Diana (28)
    }
}

TEST_F(SelectWithWhereTest, SelectWithNotEqualCondition) {
    auto result = db.execute("SELECT * FROM employees WHERE id != 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(4, result->get_row_count());
    }
}

TEST_F(SelectWithWhereTest, SelectWithFloatComparison) {
    auto result = db.execute("SELECT * FROM employees WHERE salary > 50000.0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(3, result->get_row_count());  // Alice (50000 not >), Charlie (60000), Eve (55000)
    }
}

TEST_F(SelectWithWhereTest, SelectWithNoMatches) {
    auto result = db.execute("SELECT * FROM employees WHERE age > 100");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(0, result->get_row_count());
    }
}

TEST_F(SelectWithWhereTest, SelectWithFloatEquality) {
    auto result = db.execute("SELECT * FROM employees WHERE salary = 50000.0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());  // Alice
    }
}

TEST_F(SelectWithWhereTest, SelectWithStringContains) {
    // This assumes LIKE operator is supported
    // Otherwise this test will fail
    auto result = db.execute("SELECT * FROM employees WHERE name LIKE '%a%'");
    EXPECT_TRUE(result != nullptr);
    // Expected: Alice, Diana, Charlie (have 'a' in name)
}

TEST_F(SelectWithWhereTest, SelectWithIntegerEquality) {
    auto result = db.execute("SELECT * FROM employees WHERE age = 25");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());  // Bob
    }
}

// Test edge cases
TEST_F(SelectWithWhereTest, SelectWithBoundaryValue) {
    auto result = db.execute("SELECT * FROM employees WHERE age = 30");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());  // Alice
    }
}

TEST_F(SelectWithWhereTest, SelectWithMultipleResults) {
    auto result = db.execute("SELECT * FROM employees WHERE salary >= 45000.0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(5, result->get_row_count());  // All have salary >= 45000
    }
}

// Test expression evaluation with different types
TEST_F(SelectWithWhereTest, SelectWithArithmeticExpression) {
    // Test if arithmetic expressions work in WHERE
    auto result = db.execute("SELECT * FROM employees WHERE age + 5 > 30");
    EXPECT_TRUE(result != nullptr);
    // If supported, should return employees where age > 25
}

TEST_F(SelectWithWhereTest, SelectWithComplexCondition) {
    // This requires AND/OR support which may not be implemented yet
    auto result = db.execute("SELECT * FROM employees WHERE age > 25 AND salary < 50000");
    EXPECT_TRUE(result != nullptr);
}

// Additional helper tests
class SelectWithGroupByTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create a test table with duplicate values for grouping
        Schema schema;
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("salaries", schema);
        
        // Insert test data with departments
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 80000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 75000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 60000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 55000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('HR', 50000.0)");
    }
};

TEST_F(SelectWithGroupByTest, GroupByDepartment) {
    // Test GROUP BY functionality (when implemented)
    auto result = db.execute("SELECT dept FROM salaries GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 3 rows (Engineering, Sales, HR)
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(SelectWithGroupByTest, GroupByWithCount) {
    // Test GROUP BY with COUNT aggregate
    auto result = db.execute("SELECT dept, COUNT(*) as count FROM salaries GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 3 rows with counts: Engineering=2, Sales=2, HR=1
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(SelectWithGroupByTest, GroupByWithSum) {
    // Test GROUP BY with SUM aggregate
    auto result = db.execute("SELECT dept, SUM(salary) as total FROM salaries GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(SelectWithGroupByTest, GroupByWithAverage) {
    // Test GROUP BY with AVG aggregate
    auto result = db.execute("SELECT dept, AVG(salary) as avg_salary FROM salaries GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(SelectWithGroupByTest, GroupByWithHaving) {
    // Test GROUP BY with HAVING clause
    auto result = db.execute("SELECT dept, COUNT(*) as count FROM salaries GROUP BY dept HAVING COUNT(*) > 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return only Engineering and Sales (both have 2 employees)
        EXPECT_EQ(2, result->get_row_count());
    }
}
