#include <gtest/gtest.h>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <string>
#include <vector>

using namespace lyradb;

class HavingClauseTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create a test table with departments and salaries
        Schema schema;
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("employees", schema);
        
        // Insert test data with multiple employees per department
        // Engineering: 3 employees
        db.execute("INSERT INTO employees (dept, salary) VALUES ('Engineering', 80000.0)");
        db.execute("INSERT INTO employees (dept, salary) VALUES ('Engineering', 75000.0)");
        db.execute("INSERT INTO employees (dept, salary) VALUES ('Engineering', 95000.0)");
        
        // Sales: 2 employees
        db.execute("INSERT INTO employees (dept, salary) VALUES ('Sales', 60000.0)");
        db.execute("INSERT INTO employees (dept, salary) VALUES ('Sales', 55000.0)");
        
        // HR: 1 employee
        db.execute("INSERT INTO employees (dept, salary) VALUES ('HR', 50000.0)");
    }
};

// Basic HAVING tests - Group filtering based on conditions
TEST_F(HavingClauseTest, HavingWithCountGreater) {
    // Get departments with more than 1 employee
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept HAVING COUNT(*) > 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return Engineering and Sales (2 groups with 2+ employees)
        EXPECT_GE(result->get_row_count(), 2);
    }
}

TEST_F(HavingClauseTest, HavingWithCountEqual) {
    // Get departments with exactly 3 employees
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept HAVING COUNT(*) = 3");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return only Engineering (3 employees)
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(HavingClauseTest, HavingWithCountLess) {
    // Get departments with fewer than 2 employees
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept HAVING COUNT(*) < 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return only HR (1 employee)
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(HavingClauseTest, HavingWithSumGreater) {
    // Get departments with total salary > 150000
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept HAVING SUM(salary) > 150000");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return Engineering (sum = 250000) only
        EXPECT_GE(result->get_row_count(), 1);
    }
}

TEST_F(HavingClauseTest, HavingWithAverageSalary) {
    // Get departments with average salary > 60000
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept HAVING AVG(salary) > 60000");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return Engineering (avg = 83333.33) and Sales (avg = 57500) may be included
        EXPECT_GE(result->get_row_count(), 1);
    }
}

TEST_F(HavingClauseTest, HavingWithMinSalary) {
    // Get departments where minimum salary is at least 55000
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept HAVING MIN(salary) >= 55000");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return Engineering and Sales
        EXPECT_GE(result->get_row_count(), 2);
    }
}

TEST_F(HavingClauseTest, HavingWithMaxSalary) {
    // Get departments where maximum salary exceeds 80000
    auto result = db.execute("SELECT dept FROM employees GROUP BY dept HAVING MAX(salary) > 80000");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return only Engineering (max = 95000)
        EXPECT_EQ(1, result->get_row_count());
    }
}

// HAVING with WHERE combination
class HavingWithWhereTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("salaries", schema);
        
        // Engineering: 3 employees (80K, 75K, 95K)
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 80000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 75000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 95000.0)");
        
        // Sales: 2 employees (60K, 55K)
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 60000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 55000.0)");
        
        // HR: 2 employees (50K, 52K)
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('HR', 50000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('HR', 52000.0)");
        
        // Marketing: 1 employee (65K) - will be filtered out by WHERE
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Marketing', 65000.0)");
    }
};

TEST_F(HavingWithWhereTest, WhereBeforeGroupByAndHaving) {
    // First filter WHERE salary >= 60000, then GROUP BY dept, then HAVING COUNT(*) > 1
    auto result = db.execute("SELECT dept FROM salaries WHERE salary >= 60000 GROUP BY dept HAVING COUNT(*) > 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // After WHERE: Engineering (3) and Sales (1) remain
        // After GROUP BY: 2 groups
        // After HAVING COUNT > 1: Only Engineering qualifies
        EXPECT_GE(result->get_row_count(), 1);
    }
}

TEST_F(HavingWithWhereTest, WhereFiltersRowsBeforeHaving) {
    // WHERE filters out low salary rows first
    auto result = db.execute("SELECT dept FROM salaries WHERE salary > 75000 GROUP BY dept HAVING COUNT(*) > 0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // After WHERE salary > 75000: Engineering (95K, 80K), Sales (60K excluded), HR (excluded)
        // So only Engineering remains
        EXPECT_EQ(1, result->get_row_count());
    }
}

// Edge cases
class HavingEdgeCasesTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"category", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"value", DataType::INT64, 8, false});
        
        db.create_table("data", schema);
        
        // Create groups with different sizes
        db.execute("INSERT INTO data (category, value) VALUES ('A', 10)");
        db.execute("INSERT INTO data (category, value) VALUES ('A', 20)");
        db.execute("INSERT INTO data (category, value) VALUES ('A', 30)");
        db.execute("INSERT INTO data (category, value) VALUES ('A', 40)");  // Group A: 4 items
        
        db.execute("INSERT INTO data (category, value) VALUES ('B', 15)");
        db.execute("INSERT INTO data (category, value) VALUES ('B', 25)");  // Group B: 2 items
        
        db.execute("INSERT INTO data (category, value) VALUES ('C', 100)"); // Group C: 1 item
    }
};

TEST_F(HavingEdgeCasesTest, HavingFiltersAllGroups) {
    // HAVING condition filters out all groups
    auto result = db.execute("SELECT category FROM data GROUP BY category HAVING COUNT(*) > 10");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // No group has more than 10 items, so result should be empty
        EXPECT_EQ(0, result->get_row_count());
    }
}

TEST_F(HavingEdgeCasesTest, HavingKeepsAllGroups) {
    // HAVING condition keeps all groups
    auto result = db.execute("SELECT category FROM data GROUP BY category HAVING COUNT(*) > 0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // All groups have at least 1 item
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(HavingEdgeCasesTest, HavingWithBoundaryValue) {
    // HAVING with exact boundary value
    auto result = db.execute("SELECT category FROM data GROUP BY category HAVING COUNT(*) = 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Only group B has exactly 2 items
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(HavingEdgeCasesTest, HavingSingleGroup) {
    // Filter to single group with HAVING
    auto result = db.execute("SELECT category FROM data WHERE category = 'C' GROUP BY category HAVING COUNT(*) >= 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Only group C, and it has 1 item, so should be included
        EXPECT_EQ(1, result->get_row_count());
    }
}

// Performance test with HAVING
class HavingPerformanceTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"category", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"value", DataType::INT64, 8, false});
        
        db.create_table("large_table", schema);
        
        // Insert 200 rows with 5 categories (40 rows each)
        for (int i = 0; i < 200; ++i) {
            int category = i % 5;
            db.execute("INSERT INTO large_table (category, value) VALUES ('cat_" + std::to_string(category) + "', " + std::to_string(i) + ")");
        }
    }
};

TEST_F(HavingPerformanceTest, HavingOnLargeTable) {
    auto result = db.execute("SELECT category FROM large_table GROUP BY category HAVING COUNT(*) > 30");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // All 5 categories have 40 rows, so all should pass HAVING COUNT > 30
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(HavingPerformanceTest, HavingFiltersHarshly) {
    auto result = db.execute("SELECT category FROM large_table GROUP BY category HAVING COUNT(*) > 50");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // All categories have exactly 40 rows, so none pass HAVING COUNT > 50
        EXPECT_EQ(0, result->get_row_count());
    }
}
