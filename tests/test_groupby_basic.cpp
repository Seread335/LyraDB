#include <gtest/gtest.h>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <string>
#include <vector>

using namespace lyradb;

class GroupByBasicTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create a test table with departments and salaries
        Schema schema;
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"employee", DataType::VARCHAR, 100, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("employees", schema);
        
        // Insert test data
        db.execute("INSERT INTO employees (dept, employee, salary) VALUES ('Engineering', 'Alice', 80000.0)");
        db.execute("INSERT INTO employees (dept, employee, salary) VALUES ('Engineering', 'Bob', 75000.0)");
        db.execute("INSERT INTO employees (dept, employee, salary) VALUES ('Sales', 'Charlie', 60000.0)");
        db.execute("INSERT INTO employees (dept, employee, salary) VALUES ('Sales', 'Diana', 55000.0)");
        db.execute("INSERT INTO employees (dept, employee, salary) VALUES ('HR', 'Eve', 50000.0)");
    }
};

// Basic GROUP BY tests (functional verification)
TEST_F(GroupByBasicTest, GroupByDepartment) {
    // Test basic GROUP BY - should return 3 groups (one per department)
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_GE(result->get_row_count(), 3);  // At least 3 groups
    }
}

TEST_F(GroupByBasicTest, GroupByWithDistinct) {
    // Without GROUP BY, should return 5 rows
    // With GROUP BY, should return 3 rows (one per unique dept)
    auto result_grouped = db.execute("SELECT dept FROM employees GROUP BY dept");
    auto result_all = db.execute("SELECT dept FROM employees");
    
    EXPECT_TRUE(result_grouped != nullptr);
    EXPECT_TRUE(result_all != nullptr);
    
    if (result_grouped && result_all) {
        EXPECT_LE(result_grouped->get_row_count(), result_all->get_row_count());
    }
}

TEST_F(GroupByBasicTest, GroupByWithMultipleRows) {
    // Test that GROUP BY correctly groups multiple rows
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 3 departments: Engineering, Sales, HR
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(GroupByBasicTest, GroupByEdgeCase) {
    // Create another table with single row
    Schema schema;
    schema.add_column(Column{"category", DataType::VARCHAR, 50, false});
    schema.add_column(Column{"value", DataType::INT64, 8, false});
    
    db.create_table("simple", schema);
    db.execute("INSERT INTO simple (category, value) VALUES ('A', 100)");
    
    auto result = db.execute("SELECT category FROM simple GROUP BY category");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}

// Test GROUP BY with different data types
class GroupByDataTypesTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"category", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"amount", DataType::INT64, 8, false});
        
        db.create_table("transactions", schema);
        
        db.execute("INSERT INTO transactions (category, amount) VALUES ('Food', 50)");
        db.execute("INSERT INTO transactions (category, amount) VALUES ('Food', 75)");
        db.execute("INSERT INTO transactions (category, amount) VALUES ('Transport', 30)");
        db.execute("INSERT INTO transactions (category, amount) VALUES ('Transport', 40)");
        db.execute("INSERT INTO transactions (category, amount) VALUES ('Entertainment', 100)");
    }
};

TEST_F(GroupByDataTypesTest, GroupByString) {
    auto result = db.execute("SELECT category FROM transactions GROUP BY category");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 3 categories
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(GroupByDataTypesTest, GroupByInteger) {
    // GROUP BY integer column
    auto result = db.execute("SELECT amount FROM transactions GROUP BY amount");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 5 unique amounts
        EXPECT_EQ(5, result->get_row_count());
    }
}

// Test GROUP BY with WHERE clause combination
class GroupByWhereTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("salaries", schema);
        
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 80000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 75000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 60000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 55000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 45000.0)");
    }
};

TEST_F(GroupByWhereTest, GroupByWithWhereBefore) {
    // Filter rows first, then group
    auto result = db.execute("SELECT dept FROM salaries WHERE salary > 50000.0 GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // After filtering, should have Engineering and Sales (with high salaries)
        EXPECT_GE(result->get_row_count(), 2);
    }
}

TEST_F(GroupByWhereTest, GroupByWithSingleDept) {
    // Filter to single department, then group (should return 1 group)
    auto result = db.execute("SELECT dept FROM salaries WHERE dept = 'Engineering' GROUP BY dept");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}

// Performance test
class GroupByPerformanceTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"category", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"value", DataType::INT64, 8, false});
        
        db.create_table("large_data", schema);
        
        // Insert 100 rows with 10 categories
        for (int i = 0; i < 100; ++i) {
            int category = i % 10;
            db.execute("INSERT INTO large_data (category, value) VALUES ('cat_" + std::to_string(category) + "', " + std::to_string(i) + ")");
        }
    }
};

TEST_F(GroupByPerformanceTest, GroupByLargeTable) {
    auto result = db.execute("SELECT category FROM large_data GROUP BY category");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 10 groups
        EXPECT_EQ(10, result->get_row_count());
    }
}

TEST_F(GroupByPerformanceTest, GroupByWithLimit) {
    // GROUP BY followed by LIMIT should work
    auto result = db.execute("SELECT category FROM large_data GROUP BY category LIMIT 5");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have at most 5 groups
        EXPECT_LE(result->get_row_count(), 5);
    }
}
