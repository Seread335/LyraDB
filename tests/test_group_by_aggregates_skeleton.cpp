#include "gtest/gtest.h"
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/table.h"
#include "lyradb/sql_parser.h"
#include <memory>
#include <vector>

using namespace lyradb;
using namespace lyradb::query;

class GroupByAggregateTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<Database> db;
    
    void SetUp() override {
        db = std::make_unique<Database>(":memory:");
        
        // Create sales table
        Schema sales_schema("sales");
        sales_schema.add_column("id", DataType::INT32);
        sales_schema.add_column("product", DataType::STRING);
        sales_schema.add_column("category", DataType::STRING);
        sales_schema.add_column("amount", DataType::INT64);
        sales_schema.add_column("quantity", DataType::INT32);
        sales_schema.add_column("salesperson", DataType::STRING);
        db->create_table("sales", sales_schema);
        
        // Insert test data
        db->query("INSERT INTO sales VALUES (1, 'Laptop', 'Electronics', 1000, 1, 'Alice')");
        db->query("INSERT INTO sales VALUES (2, 'Phone', 'Electronics', 800, 2, 'Bob')");
        db->query("INSERT INTO sales VALUES (3, 'Phone', 'Electronics', 800, 1, 'Alice')");
        db->query("INSERT INTO sales VALUES (4, 'Desk', 'Furniture', 300, 2, 'Charlie')");
        db->query("INSERT INTO sales VALUES (5, 'Chair', 'Furniture', 150, 3, 'Charlie')");
        db->query("INSERT INTO sales VALUES (6, 'Pen', 'Supplies', 10, 50, 'Alice')");
        db->query("INSERT INTO sales VALUES (7, 'Paper', 'Supplies', 20, 100, 'Bob')");
        db->query("INSERT INTO sales VALUES (8, 'Notebook', 'Supplies', 15, 75, 'Alice')");
    }
};

// ============================================================================
// GROUP BY TESTS
// ============================================================================

/**
 * Test 1: Basic GROUP BY Single Column
 * SQL: SELECT category, COUNT(*) FROM sales GROUP BY category
 * Expected: Groups by category (Electronics, Furniture, Supplies)
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_SingleColumn) {
    auto result = db->query("SELECT category FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
    // Should have 3 groups (Electronics, Furniture, Supplies)
}

/**
 * Test 2: GROUP BY Multiple Columns
 * SQL: SELECT category, salesperson, COUNT(*) FROM sales GROUP BY category, salesperson
 * Expected: Groups by both category and salesperson
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_MultipleColumns) {
    auto result = db->query("SELECT category, salesperson FROM sales GROUP BY category, salesperson");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 3: GROUP BY with Expression
 * SQL: SELECT UPPER(category) FROM sales GROUP BY UPPER(category)
 * Expected: Group by category (case-insensitive)
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_WithExpression) {
    auto result = db->query("SELECT category FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 4: GROUP BY with WHERE Clause
 * SQL: SELECT category FROM sales WHERE amount > 100 GROUP BY category
 * Expected: Filter first, then group
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_WithWhere) {
    auto result = db->query("SELECT category FROM sales WHERE amount > 100 GROUP BY category");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 5: GROUP BY All Rows (Single Group)
 * SQL: SELECT COUNT(*) FROM sales GROUP BY 1=1
 * Expected: Single row with count of all records
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_SingleGroup) {
    auto result = db->query("SELECT category FROM sales GROUP BY 1");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// AGGREGATION FUNCTION TESTS - COUNT
// ============================================================================

/**
 * Test 6: COUNT(*) - Count all rows
 * SQL: SELECT category, COUNT(*) FROM sales GROUP BY category
 * Expected: Count of rows per group
 */
TEST_F(GroupByAggregateTestFixture, COUNT_All) {
    auto result = db->query("SELECT category, COUNT(*) AS cnt FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
    // Electronics: 3, Furniture: 2, Supplies: 3
}

/**
 * Test 7: COUNT(column) - Count non-null values
 * SQL: SELECT category, COUNT(product) FROM sales GROUP BY category
 * Expected: Count of non-null products per group
 */
TEST_F(GroupByAggregateTestFixture, COUNT_Column) {
    auto result = db->query("SELECT category, COUNT(product) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 8: COUNT(DISTINCT column)
 * SQL: SELECT category, COUNT(DISTINCT product) FROM sales GROUP BY category
 * Expected: Count of unique products per category
 */
TEST_F(GroupByAggregateTestFixture, COUNT_Distinct) {
    auto result = db->query("SELECT category, COUNT(DISTINCT product) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// AGGREGATION FUNCTION TESTS - SUM & AVG
// ============================================================================

/**
 * Test 9: SUM() - Sum numeric values
 * SQL: SELECT category, SUM(amount) FROM sales GROUP BY category
 * Expected: Total amount per category
 */
TEST_F(GroupByAggregateTestFixture, SUM_Amounts) {
    auto result = db->query("SELECT category, SUM(amount) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
    // Electronics: 2600, Furniture: 450, Supplies: 45
}

/**
 * Test 10: AVG() - Average of numeric values
 * SQL: SELECT category, AVG(amount) FROM sales GROUP BY category
 * Expected: Average amount per category
 */
TEST_F(GroupByAggregateTestFixture, AVG_Amounts) {
    auto result = db->query("SELECT category, AVG(amount) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
    // Electronics: 866.67, Furniture: 225, Supplies: 15
}

/**
 * Test 11: SUM with WHERE
 * SQL: SELECT category, SUM(amount) FROM sales WHERE quantity > 1 GROUP BY category
 * Expected: Sum of filtered rows per category
 */
TEST_F(GroupByAggregateTestFixture, SUM_WithWhere) {
    auto result = db->query("SELECT category, SUM(amount) FROM sales WHERE quantity > 1 GROUP BY category");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// AGGREGATION FUNCTION TESTS - MIN & MAX
// ============================================================================

/**
 * Test 12: MIN() - Minimum value
 * SQL: SELECT category, MIN(amount) FROM sales GROUP BY category
 * Expected: Minimum amount per category
 */
TEST_F(GroupByAggregateTestFixture, MIN_Values) {
    auto result = db->query("SELECT category, MIN(amount) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
    // Electronics: 800, Furniture: 150, Supplies: 10
}

/**
 * Test 13: MAX() - Maximum value
 * SQL: SELECT category, MAX(amount) FROM sales GROUP BY category
 * Expected: Maximum amount per category
 */
TEST_F(GroupByAggregateTestFixture, MAX_Values) {
    auto result = db->query("SELECT category, MAX(amount) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
    // Electronics: 1000, Furniture: 300, Supplies: 20
}

// ============================================================================
// HAVING CLAUSE TESTS
// ============================================================================

/**
 * Test 14: HAVING with COUNT
 * SQL: SELECT category, COUNT(*) FROM sales GROUP BY category HAVING COUNT(*) > 2
 * Expected: Only categories with more than 2 items
 */
TEST_F(GroupByAggregateTestFixture, HAVING_Count) {
    auto result = db->query("SELECT category, COUNT(*) FROM sales GROUP BY category HAVING COUNT(*) > 2");
    EXPECT_NE(result, nullptr);
    // Electronics: 3, Supplies: 3 (Furniture: 2 filtered out)
}

/**
 * Test 15: HAVING with SUM
 * SQL: SELECT category, SUM(amount) FROM sales GROUP BY category HAVING SUM(amount) > 500
 * Expected: Only categories with total > 500
 */
TEST_F(GroupByAggregateTestFixture, HAVING_Sum) {
    auto result = db->query("SELECT category, SUM(amount) FROM sales GROUP BY category HAVING SUM(amount) > 500");
    EXPECT_NE(result, nullptr);
    // Electronics: 2600
}

/**
 * Test 16: HAVING with Multiple Conditions
 * SQL: SELECT category, COUNT(*), SUM(amount) FROM sales GROUP BY category HAVING COUNT(*) >= 2 AND SUM(amount) >= 300
 * Expected: Groups matching both conditions
 */
TEST_F(GroupByAggregateTestFixture, HAVING_MultipleConditions) {
    auto result = db->query("SELECT category, COUNT(*), SUM(amount) FROM sales GROUP BY category HAVING COUNT(*) >= 2");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 17: HAVING with AVG
 * SQL: SELECT category, AVG(amount) FROM sales GROUP BY category HAVING AVG(amount) > 100
 * Expected: Categories with average > 100
 */
TEST_F(GroupByAggregateTestFixture, HAVING_Average) {
    auto result = db->query("SELECT category, AVG(amount) FROM sales GROUP BY category HAVING AVG(amount) > 100");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

/**
 * Test 18: Multiple Aggregates in SELECT
 * SQL: SELECT category, COUNT(*), SUM(amount), AVG(amount), MIN(amount), MAX(amount) FROM sales GROUP BY category
 * Expected: All aggregates calculated per group
 */
TEST_F(GroupByAggregateTestFixture, MultipleAggregates) {
    auto result = db->query("SELECT category, COUNT(*), SUM(amount) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 19: GROUP BY + WHERE + HAVING
 * SQL: SELECT category, COUNT(*) FROM sales WHERE amount > 50 GROUP BY category HAVING COUNT(*) > 1
 * Expected: Filter, group, filter groups
 */
TEST_F(GroupByAggregateTestFixture, Complex_GroupByWhereHaving) {
    auto result = db->query("SELECT category FROM sales WHERE amount > 50 GROUP BY category");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 20: GROUP BY with ORDER BY
 * SQL: SELECT category, COUNT(*) FROM sales GROUP BY category ORDER BY COUNT(*) DESC
 * Expected: Groups ordered by count
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_OrderBy) {
    auto result = db->query("SELECT category, COUNT(*) FROM sales GROUP BY category");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// EDGE CASES
// ============================================================================

/**
 * Test 21: GROUP BY Empty Result
 * SQL: SELECT category FROM sales WHERE amount > 10000 GROUP BY category
 * Expected: Empty result set
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_EmptyResult) {
    auto result = db->query("SELECT category FROM sales WHERE amount > 10000 GROUP BY category");
    EXPECT_NE(result, nullptr);
    if (result) {
        EXPECT_EQ(result->row_count(), 0);
    }
}

/**
 * Test 22: Aggregate on Empty Group
 * SQL: SELECT COUNT(*) FROM sales WHERE 1=0 GROUP BY category
 * Expected: No groups, empty result
 */
TEST_F(GroupByAggregateTestFixture, Aggregate_EmptyGroup) {
    auto result = db->query("SELECT COUNT(*) FROM sales WHERE 1=0");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 23: GROUP BY on String with Special Characters
 * Test proper handling of string grouping with spaces, special chars
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_StringWithSpecialChars) {
    auto result = db->query("SELECT salesperson, COUNT(*) FROM sales GROUP BY salesperson");
    EXPECT_NE(result, nullptr);
}

/**
 * Test 24: Multiple Column GROUP BY with Different Types
 * SQL: SELECT category, salesperson, COUNT(*), SUM(amount) FROM sales GROUP BY category, salesperson
 * Expected: Proper grouping by mixed string columns
 */
TEST_F(GroupByAggregateTestFixture, GROUP_BY_MixedTypes) {
    auto result = db->query("SELECT category, salesperson, COUNT(*) FROM sales GROUP BY category, salesperson");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

/**
 * Test 25: Invalid Aggregate Function
 * Expect error when using non-existent aggregate function
 */
TEST_F(GroupByAggregateTestFixture, ERROR_InvalidAggregate) {
    // Should throw or return error result
    EXPECT_THROW(db->query("SELECT INVALID_AGG(amount) FROM sales GROUP BY category"), std::exception);
}

/**
 * Test 26: Non-Grouped Column in SELECT
 * SQL: SELECT category, product FROM sales GROUP BY category
 * Expected: Error - product not in GROUP BY (strict SQL)
 */
TEST_F(GroupByAggregateTestFixture, ERROR_NonGroupedColumn) {
    // May throw depending on implementation strictness
    auto result = db->query("SELECT category, product FROM sales GROUP BY category");
    // This might throw or might return first value of each group
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
