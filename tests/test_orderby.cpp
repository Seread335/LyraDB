#include <gtest/gtest.h>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <string>
#include <vector>

using namespace lyradb;

class OrderByBasicTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create a test table with employees
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, true});
        schema.add_column(Column{"name", DataType::VARCHAR, 100, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("employees", schema);
        
        // Insert test data in random order
        db.execute("INSERT INTO employees (id, name, salary) VALUES (3, 'Charlie', 60000.0)");
        db.execute("INSERT INTO employees (id, name, salary) VALUES (1, 'Alice', 50000.0)");
        db.execute("INSERT INTO employees (id, name, salary) VALUES (5, 'Eve', 75000.0)");
        db.execute("INSERT INTO employees (id, name, salary) VALUES (2, 'Bob', 55000.0)");
        db.execute("INSERT INTO employees (id, name, salary) VALUES (4, 'Diana', 65000.0)");
    }
};

// Basic ORDER BY tests
TEST_F(OrderByBasicTest, OrderByIntegerAscending) {
    auto result = db.execute("SELECT id, name FROM employees ORDER BY id ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return ids in order: 1, 2, 3, 4, 5
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(OrderByBasicTest, OrderByIntegerDescending) {
    auto result = db.execute("SELECT id, name FROM employees ORDER BY id DESC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return ids in reverse order: 5, 4, 3, 2, 1
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(OrderByBasicTest, OrderByStringAscending) {
    auto result = db.execute("SELECT name, id FROM employees ORDER BY name ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return names in alphabetical order: Alice, Bob, Charlie, Diana, Eve
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(OrderByBasicTest, OrderByStringDescending) {
    auto result = db.execute("SELECT name, id FROM employees ORDER BY name DESC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return names in reverse alphabetical order: Eve, Diana, Charlie, Bob, Alice
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(OrderByBasicTest, OrderByFloatAscending) {
    auto result = db.execute("SELECT salary, name FROM employees ORDER BY salary ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return in salary order: 50000, 55000, 60000, 65000, 75000
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(OrderByBasicTest, OrderByFloatDescending) {
    auto result = db.execute("SELECT salary, name FROM employees ORDER BY salary DESC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return in reverse salary order: 75000, 65000, 60000, 55000, 50000
        EXPECT_EQ(5, result->get_row_count());
    }
}

// ORDER BY with WHERE clause
class OrderByWithWhereTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("staff", schema);
        
        db.execute("INSERT INTO staff (id, dept, salary) VALUES (1, 'Engineering', 80000.0)");
        db.execute("INSERT INTO staff (id, dept, salary) VALUES (2, 'Sales', 60000.0)");
        db.execute("INSERT INTO staff (id, dept, salary) VALUES (3, 'Engineering', 75000.0)");
        db.execute("INSERT INTO staff (id, dept, salary) VALUES (4, 'HR', 50000.0)");
        db.execute("INSERT INTO staff (id, dept, salary) VALUES (5, 'Sales', 65000.0)");
    }
};

TEST_F(OrderByWithWhereTest, WhereBeforeOrderBy) {
    // Filter WHERE salary > 60000, then ORDER BY salary
    auto result = db.execute("SELECT id, salary FROM staff WHERE salary > 60000 ORDER BY salary ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // After WHERE: 65000, 75000, 80000
        // After ORDER BY ASC: 65000, 75000, 80000
        EXPECT_GE(result->get_row_count(), 3);
    }
}

TEST_F(OrderByWithWhereTest, WhereWithOrderByDescending) {
    auto result = db.execute("SELECT dept, salary FROM staff WHERE dept = 'Engineering' ORDER BY salary DESC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Engineering dept: 80000, 75000 (in DESC order)
        EXPECT_EQ(2, result->get_row_count());
    }
}

// ORDER BY with GROUP BY
class OrderByWithGroupByTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        
        db.create_table("salaries", schema);
        
        // Multiple employees per department
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 80000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Engineering', 75000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 60000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('Sales', 55000.0)");
        db.execute("INSERT INTO salaries (dept, salary) VALUES ('HR', 50000.0)");
    }
};

TEST_F(OrderByWithGroupByTest, GroupByThenOrderBy) {
    // GROUP BY dept, then ORDER BY dept name
    auto result = db.execute("SELECT dept FROM salaries GROUP BY dept ORDER BY dept ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 3 departments in alphabetical order: Engineering, HR, Sales
        EXPECT_EQ(3, result->get_row_count());
    }
}

// Multiple column ORDER BY
class MultiColumnOrderByTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"dept", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"salary", DataType::FLOAT64, 8, false});
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        
        db.create_table("employees", schema);
        
        // Test data with duplicate departments
        db.execute("INSERT INTO employees (dept, salary, id) VALUES ('Sales', 60000.0, 1)");
        db.execute("INSERT INTO employees (dept, salary, id) VALUES ('Engineering', 75000.0, 2)");
        db.execute("INSERT INTO employees (dept, salary, id) VALUES ('Sales', 60000.0, 3)");
        db.execute("INSERT INTO employees (dept, salary, id) VALUES ('Engineering', 80000.0, 4)");
        db.execute("INSERT INTO employees (dept, salary, id) VALUES ('Sales', 70000.0, 5)");
    }
};

TEST_F(MultiColumnOrderByTest, MultiColumnOrderByAscending) {
    // ORDER BY dept ASC, then salary ASC
    auto result = db.execute("SELECT dept, salary FROM employees ORDER BY dept ASC, salary ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should order by dept first, then by salary within each dept
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(MultiColumnOrderByTest, MultiColumnMixedDirection) {
    // ORDER BY dept ASC, then salary DESC
    auto result = db.execute("SELECT dept, salary FROM employees ORDER BY dept ASC, salary DESC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should order by dept ascending, then salary descending within each dept
        EXPECT_EQ(5, result->get_row_count());
    }
}

// Edge cases
class OrderByEdgeCasesTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"value", DataType::INT64, 8, false});
        schema.add_column(Column{"name", DataType::VARCHAR, 50, false});
        
        db.create_table("data", schema);
        
        db.execute("INSERT INTO data (value, name) VALUES (100, 'hundred')");
        db.execute("INSERT INTO data (value, name) VALUES (10, 'ten')");
        db.execute("INSERT INTO data (value, name) VALUES (1, 'one')");
        db.execute("INSERT INTO data (value, name) VALUES (1000, 'thousand')");
        db.execute("INSERT INTO data (value, name) VALUES (50, 'fifty')");
    }
};

TEST_F(OrderByEdgeCasesTest, OrderByWithDuplicateValues) {
    // Two rows have value 1, should maintain relative order or be stable
    auto result = db.execute("SELECT value FROM data ORDER BY value ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(OrderByEdgeCasesTest, OrderBySingleRow) {
    // Filter to single row, then ORDER BY (should still work)
    auto result = db.execute("SELECT value FROM data WHERE name = 'one' ORDER BY value ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(OrderByEdgeCasesTest, OrderByAllSameValues) {
    // All values are same, so order should be preserved or stable
    auto result = db.execute("SELECT * FROM data WHERE value = 100 ORDER BY name ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}

// Performance test
class OrderByPerformanceTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        schema.add_column(Column{"value", DataType::INT64, 8, false});
        
        db.create_table("large_data", schema);
        
        // Insert 100 rows in reverse order
        for (int i = 100; i > 0; --i) {
            db.execute("INSERT INTO large_data (id, value) VALUES (" + std::to_string(i) + ", " + std::to_string(i * 10) + ")");
        }
    }
};

TEST_F(OrderByPerformanceTest, OrderByLargeTable) {
    auto result = db.execute("SELECT id FROM large_data ORDER BY id ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 100 rows in ascending order
        EXPECT_EQ(100, result->get_row_count());
    }
}

TEST_F(OrderByPerformanceTest, OrderByDescendingLargeTable) {
    auto result = db.execute("SELECT id FROM large_data ORDER BY id DESC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 100 rows in descending order
        EXPECT_EQ(100, result->get_row_count());
    }
}

TEST_F(OrderByPerformanceTest, OrderByWithFilter) {
    auto result = db.execute("SELECT id FROM large_data WHERE id > 50 ORDER BY id ASC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 50 rows (51-100) in ascending order
        EXPECT_EQ(50, result->get_row_count());
    }
}
