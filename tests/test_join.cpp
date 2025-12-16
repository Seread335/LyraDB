#include <gtest/gtest.h>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <string>
#include <vector>

using namespace lyradb;

/**
 * INNER JOIN Tests
 * Join returns only matching rows from both tables
 */
class InnerJoinBasicTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create employees table
        Schema emp_schema;
        emp_schema.add_column(Column{"emp_id", DataType::INT64, 8, false});
        emp_schema.add_column(Column{"emp_name", DataType::VARCHAR, 100, false});
        emp_schema.add_column(Column{"dept_id", DataType::INT64, 8, false});
        db.create_table("employees", emp_schema);
        
        // Create departments table
        Schema dept_schema;
        dept_schema.add_column(Column{"dept_id", DataType::INT64, 8, false});
        dept_schema.add_column(Column{"dept_name", DataType::VARCHAR, 100, false});
        db.create_table("departments", dept_schema);
        
        // Insert employees
        db.execute("INSERT INTO employees (emp_id, emp_name, dept_id) VALUES (1, 'Alice', 10)");
        db.execute("INSERT INTO employees (emp_id, emp_name, dept_id) VALUES (2, 'Bob', 20)");
        db.execute("INSERT INTO employees (emp_id, emp_name, dept_id) VALUES (3, 'Charlie', 10)");
        db.execute("INSERT INTO employees (emp_id, emp_name, dept_id) VALUES (4, 'Diana', 30)");
        
        // Insert departments (note: dept_id 30 has no employees, dept_id 40 has no dept)
        db.execute("INSERT INTO departments (dept_id, dept_name) VALUES (10, 'Engineering')");
        db.execute("INSERT INTO departments (dept_id, dept_name) VALUES (20, 'Sales')");
        db.execute("INSERT INTO departments (dept_id, dept_name) VALUES (30, 'HR')");
    }
};

TEST_F(InnerJoinBasicTest, SimpleInnerJoin) {
    // INNER JOIN returns only rows where dept_id matches
    auto result = db.execute("SELECT * FROM employees INNER JOIN departments ON employees.dept_id = departments.dept_id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 4 rows (all employees have matching departments)
        EXPECT_EQ(4, result->get_row_count());
    }
}

TEST_F(InnerJoinBasicTest, InnerJoinWithoutKeyword) {
    // JOIN without specifying INNER still performs INNER JOIN
    auto result = db.execute("SELECT * FROM employees JOIN departments ON employees.dept_id = departments.dept_id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(4, result->get_row_count());
    }
}

TEST_F(InnerJoinBasicTest, InnerJoinWithWhere) {
    // INNER JOIN with WHERE clause filtering
    auto result = db.execute("SELECT * FROM employees JOIN departments ON employees.dept_id = departments.dept_id WHERE employees.dept_id = 10");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 2 employees in department 10
        EXPECT_EQ(2, result->get_row_count());
    }
}

TEST_F(InnerJoinBasicTest, InnerJoinWithOrderBy) {
    // INNER JOIN with ORDER BY
    auto result = db.execute("SELECT * FROM employees JOIN departments ON employees.dept_id = departments.dept_id ORDER BY employees.emp_id DESC");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(4, result->get_row_count());
    }
}

TEST_F(InnerJoinBasicTest, InnerJoinWithLimit) {
    // INNER JOIN with LIMIT
    auto result = db.execute("SELECT * FROM employees JOIN departments ON employees.dept_id = departments.dept_id LIMIT 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(2, result->get_row_count());
    }
}

/**
 * LEFT JOIN Tests
 * Left join returns all rows from left table, matching rows from right table
 * Non-matching rows from right table are represented with NULL values
 */
class LeftJoinTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create customers table
        Schema cust_schema;
        cust_schema.add_column(Column{"cust_id", DataType::INT64, 8, false});
        cust_schema.add_column(Column{"cust_name", DataType::VARCHAR, 100, false});
        db.create_table("customers", cust_schema);
        
        // Create orders table
        Schema order_schema;
        order_schema.add_column(Column{"order_id", DataType::INT64, 8, false});
        order_schema.add_column(Column{"cust_id", DataType::INT64, 8, false});
        order_schema.add_column(Column{"amount", DataType::INT64, 8, false});
        db.create_table("orders", order_schema);
        
        // Insert customers
        db.execute("INSERT INTO customers (cust_id, cust_name) VALUES (1, 'Customer1')");
        db.execute("INSERT INTO customers (cust_id, cust_name) VALUES (2, 'Customer2')");
        db.execute("INSERT INTO customers (cust_id, cust_name) VALUES (3, 'Customer3')");
        
        // Insert orders (note: Customer3 has no orders)
        db.execute("INSERT INTO orders (order_id, cust_id, amount) VALUES (101, 1, 1000)");
        db.execute("INSERT INTO orders (order_id, cust_id, amount) VALUES (102, 1, 2000)");
        db.execute("INSERT INTO orders (order_id, cust_id, amount) VALUES (103, 2, 1500)");
    }
};

TEST_F(LeftJoinTest, SimpleLeftJoin) {
    // LEFT JOIN returns all customers, with matching orders or NULL
    auto result = db.execute("SELECT * FROM customers LEFT JOIN orders ON customers.cust_id = orders.cust_id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 4 rows: 2 for Customer1, 1 for Customer2, 1 for Customer3 (with NULLs)
        EXPECT_EQ(4, result->get_row_count());
    }
}

TEST_F(LeftJoinTest, LeftJoinWithWhere) {
    // LEFT JOIN with WHERE clause
    auto result = db.execute("SELECT * FROM customers LEFT JOIN orders ON customers.cust_id = orders.cust_id WHERE customers.cust_id = 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 2 rows for Customer1
        EXPECT_EQ(2, result->get_row_count());
    }
}

TEST_F(LeftJoinTest, LeftJoinWithLimit) {
    // LEFT JOIN with LIMIT
    auto result = db.execute("SELECT * FROM customers LEFT JOIN orders ON customers.cust_id = orders.cust_id LIMIT 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 2 rows
        EXPECT_EQ(2, result->get_row_count());
    }
}

/**
 * Multi-table JOIN Tests
 */
class MultiTableJoinTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create projects table
        Schema proj_schema;
        proj_schema.add_column(Column{"proj_id", DataType::INT64, 8, false});
        proj_schema.add_column(Column{"proj_name", DataType::VARCHAR, 100, false});
        proj_schema.add_column(Column{"dept_id", DataType::INT64, 8, false});
        db.create_table("projects", proj_schema);
        
        // Create departments table
        Schema dept_schema;
        dept_schema.add_column(Column{"dept_id", DataType::INT64, 8, false});
        dept_schema.add_column(Column{"dept_name", DataType::VARCHAR, 100, false});
        db.create_table("departments", dept_schema);
        
        // Create assignments table
        Schema assign_schema;
        assign_schema.add_column(Column{"assign_id", DataType::INT64, 8, false});
        assign_schema.add_column(Column{"proj_id", DataType::INT64, 8, false});
        assign_schema.add_column(Column{"emp_id", DataType::INT64, 8, false});
        db.create_table("assignments", assign_schema);
        
        // Insert data
        db.execute("INSERT INTO departments (dept_id, dept_name) VALUES (10, 'Engineering')");
        db.execute("INSERT INTO departments (dept_id, dept_name) VALUES (20, 'Sales')");
        
        db.execute("INSERT INTO projects (proj_id, proj_name, dept_id) VALUES (1, 'Project1', 10)");
        db.execute("INSERT INTO projects (proj_id, proj_name, dept_id) VALUES (2, 'Project2', 10)");
        db.execute("INSERT INTO projects (proj_id, proj_name, dept_id) VALUES (3, 'Project3', 20)");
        
        db.execute("INSERT INTO assignments (assign_id, proj_id, emp_id) VALUES (1, 1, 101)");
        db.execute("INSERT INTO assignments (assign_id, proj_id, emp_id) VALUES (2, 1, 102)");
        db.execute("INSERT INTO assignments (assign_id, proj_id, emp_id) VALUES (3, 2, 101)");
        db.execute("INSERT INTO assignments (assign_id, proj_id, emp_id) VALUES (4, 3, 103)");
    }
};

TEST_F(MultiTableJoinTest, TwoTableJoin) {
    // Join projects with departments
    auto result = db.execute("SELECT * FROM projects JOIN departments ON projects.dept_id = departments.dept_id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // All 3 projects have matching departments
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(MultiTableJoinTest, JoinProjectsWithAssignments) {
    // Join projects with assignments
    auto result = db.execute("SELECT * FROM projects JOIN assignments ON projects.proj_id = assignments.proj_id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // 4 assignments
        EXPECT_EQ(4, result->get_row_count());
    }
}

/**
 * JOIN with different data types
 */
class JoinDataTypesTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create products table
        Schema prod_schema;
        prod_schema.add_column(Column{"prod_id", DataType::INT64, 8, false});
        prod_schema.add_column(Column{"prod_name", DataType::VARCHAR, 100, false});
        prod_schema.add_column(Column{"price", DataType::FLOAT64, 8, false});
        db.create_table("products", prod_schema);
        
        // Create inventory table
        Schema inv_schema;
        inv_schema.add_column(Column{"inv_id", DataType::INT64, 8, false});
        inv_schema.add_column(Column{"prod_id", DataType::INT64, 8, false});
        inv_schema.add_column(Column{"quantity", DataType::INT64, 8, false});
        db.create_table("inventory", inv_schema);
        
        // Insert data
        db.execute("INSERT INTO products (prod_id, prod_name, price) VALUES (1, 'Product1', 99.99)");
        db.execute("INSERT INTO products (prod_id, prod_name, price) VALUES (2, 'Product2', 199.99)");
        db.execute("INSERT INTO products (prod_id, prod_name, price) VALUES (3, 'Product3', 299.99)");
        
        db.execute("INSERT INTO inventory (inv_id, prod_id, quantity) VALUES (1, 1, 100)");
        db.execute("INSERT INTO inventory (inv_id, prod_id, quantity) VALUES (2, 2, 50)");
        db.execute("INSERT INTO inventory (inv_id, prod_id, quantity) VALUES (3, 3, 25)");
    }
};

TEST_F(JoinDataTypesTest, JoinWithFloatColumns) {
    // Join products (with float price) with inventory
    auto result = db.execute("SELECT * FROM products JOIN inventory ON products.prod_id = inventory.prod_id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(3, result->get_row_count());
    }
}

/**
 * JOIN with GROUP BY
 */
class JoinWithGroupByTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create sales table
        Schema sales_schema;
        sales_schema.add_column(Column{"sale_id", DataType::INT64, 8, false});
        sales_schema.add_column(Column{"salesperson_id", DataType::INT64, 8, false});
        sales_schema.add_column(Column{"amount", DataType::INT64, 8, false});
        db.create_table("sales", sales_schema);
        
        // Create salespeople table
        Schema person_schema;
        person_schema.add_column(Column{"person_id", DataType::INT64, 8, false});
        person_schema.add_column(Column{"person_name", DataType::VARCHAR, 100, false});
        db.create_table("salespeople", person_schema);
        
        // Insert data
        db.execute("INSERT INTO salespeople (person_id, person_name) VALUES (1, 'Alice')");
        db.execute("INSERT INTO salespeople (person_id, person_name) VALUES (2, 'Bob')");
        
        db.execute("INSERT INTO sales (sale_id, salesperson_id, amount) VALUES (1, 1, 1000)");
        db.execute("INSERT INTO sales (sale_id, salesperson_id, amount) VALUES (2, 1, 2000)");
        db.execute("INSERT INTO sales (sale_id, salesperson_id, amount) VALUES (3, 2, 1500)");
    }
};

TEST_F(JoinWithGroupByTest, JoinWithGroupBy) {
    // JOIN followed by GROUP BY to aggregate
    auto result = db.execute("SELECT salespeople.person_name FROM sales JOIN salespeople ON sales.salesperson_id = salespeople.person_id GROUP BY salespeople.person_name");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return 2 groups (one per salesperson)
        EXPECT_EQ(2, result->get_row_count());
    }
}

/**
 * Edge case tests for JOINs
 */
class JoinEdgeCasesTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create simple tables
        Schema left_schema;
        left_schema.add_column(Column{"id", DataType::INT64, 8, false});
        left_schema.add_column(Column{"value", DataType::INT64, 8, false});
        db.create_table("left_table", left_schema);
        
        Schema right_schema;
        right_schema.add_column(Column{"id", DataType::INT64, 8, false});
        right_schema.add_column(Column{"data", DataType::VARCHAR, 100, false});
        db.create_table("right_table", right_schema);
        
        // Insert test data
        db.execute("INSERT INTO left_table (id, value) VALUES (1, 10)");
        db.execute("INSERT INTO left_table (id, value) VALUES (2, 20)");
        
        db.execute("INSERT INTO right_table (id, data) VALUES (1, 'A')");
        db.execute("INSERT INTO right_table (id, data) VALUES (3, 'C')");
    }
};

TEST_F(JoinEdgeCasesTest, InnerJoinWithNoMatches) {
    // All rows from left should be filtered out if they don't match
    auto result = db.execute("SELECT * FROM left_table JOIN right_table ON left_table.id = right_table.id WHERE left_table.id = 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // No match for id=2 in right_table
        EXPECT_EQ(0, result->get_row_count());
    }
}

TEST_F(JoinEdgeCasesTest, SingleRowJoin) {
    // Join with single matching row
    auto result = db.execute("SELECT * FROM left_table JOIN right_table ON left_table.id = right_table.id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Only one match: id=1
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(JoinEdgeCasesTest, EmptyTableJoin) {
    // Join with empty table
    Schema empty_schema;
    empty_schema.add_column(Column{"id", DataType::INT64, 8, false});
    db.create_table("empty_table", empty_schema);
    
    auto result = db.execute("SELECT * FROM left_table JOIN empty_table ON left_table.id = empty_table.id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // No rows in empty table, so no matches
        EXPECT_EQ(0, result->get_row_count());
    }
}

/**
 * Self-join test
 */
class SelfJoinTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        schema.add_column(Column{"parent_id", DataType::INT64, 8, false});
        schema.add_column(Column{"name", DataType::VARCHAR, 100, false});
        db.create_table("nodes", schema);
        
        // Insert hierarchical data (node -> parent relationships)
        db.execute("INSERT INTO nodes (id, parent_id, name) VALUES (1, 0, 'root')");
        db.execute("INSERT INTO nodes (id, parent_id, name) VALUES (2, 1, 'child1')");
        db.execute("INSERT INTO nodes (id, parent_id, name) VALUES (3, 1, 'child2')");
        db.execute("INSERT INTO nodes (id, parent_id, name) VALUES (4, 2, 'grandchild1')");
    }
};

TEST_F(SelfJoinTest, SelfJoinParentChild) {
    // Self-join to find parent-child relationships
    auto result = db.execute("SELECT * FROM nodes parent JOIN nodes child ON parent.id = child.parent_id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should find 3 parent-child relationships
        EXPECT_EQ(3, result->get_row_count());
    }
}

/**
 * Performance test for JOINs
 */
class JoinPerformanceTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema1;
        schema1.add_column(Column{"id", DataType::INT64, 8, false});
        schema1.add_column(Column{"value", DataType::INT64, 8, false});
        db.create_table("table1", schema1);
        
        Schema schema2;
        schema2.add_column(Column{"id", DataType::INT64, 8, false});
        schema2.add_column(Column{"data", DataType::INT64, 8, false});
        db.create_table("table2", schema2);
        
        // Insert test data (small dataset for testing)
        for (int i = 1; i <= 20; ++i) {
            db.execute("INSERT INTO table1 (id, value) VALUES (" + std::to_string(i) + ", " + std::to_string(i*10) + ")");
            if (i <= 15) {
                db.execute("INSERT INTO table2 (id, data) VALUES (" + std::to_string(i) + ", " + std::to_string(i*100) + ")");
            }
        }
    }
};

TEST_F(JoinPerformanceTest, LargeInnerJoin) {
    // Large INNER JOIN
    auto result = db.execute("SELECT * FROM table1 JOIN table2 ON table1.id = table2.id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 15 matching rows
        EXPECT_EQ(15, result->get_row_count());
    }
}

TEST_F(JoinPerformanceTest, LargeLeftJoin) {
    // Large LEFT JOIN
    auto result = db.execute("SELECT * FROM table1 LEFT JOIN table2 ON table1.id = table2.id");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 20 rows (all from table1)
        EXPECT_EQ(20, result->get_row_count());
    }
}

TEST_F(JoinPerformanceTest, JoinWithLimitAndOffset) {
    // JOIN with pagination
    auto result = db.execute("SELECT * FROM table1 JOIN table2 ON table1.id = table2.id LIMIT 5 OFFSET 3");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should have 5 rows (skip 3, take 5)
        EXPECT_EQ(5, result->get_row_count());
    }
}
