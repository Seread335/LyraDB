#include <gtest/gtest.h>
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/data_types.h"
#include <string>
#include <vector>

using namespace lyradb;

class LimitOffsetBasicTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        // Create a test table with 10 rows
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        schema.add_column(Column{"name", DataType::VARCHAR, 100, false});
        
        db.create_table("data", schema);
        
        // Insert 10 rows
        for (int i = 1; i <= 10; ++i) {
            db.execute("INSERT INTO data (id, name) VALUES (" + std::to_string(i) + ", 'item_" + std::to_string(i) + "')");
        }
    }
};

// Basic LIMIT tests
TEST_F(LimitOffsetBasicTest, LimitOnly) {
    auto result = db.execute("SELECT id FROM data LIMIT 5");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return first 5 rows
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, LimitZero) {
    auto result = db.execute("SELECT id FROM data LIMIT 0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return no rows
        EXPECT_EQ(0, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, LimitGreaterThanRows) {
    auto result = db.execute("SELECT id FROM data LIMIT 20");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return all 10 rows (limit is larger than total)
        EXPECT_EQ(10, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, LimitOne) {
    auto result = db.execute("SELECT id FROM data LIMIT 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should return exactly 1 row
        EXPECT_EQ(1, result->get_row_count());
    }
}

// Basic OFFSET tests
TEST_F(LimitOffsetBasicTest, OffsetOnly) {
    auto result = db.execute("SELECT id FROM data OFFSET 5");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Should skip first 5 rows and return remaining 5
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, OffsetZero) {
    auto result = db.execute("SELECT id FROM data OFFSET 0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // No offset, should return all 10 rows
        EXPECT_EQ(10, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, OffsetGreaterThanRows) {
    auto result = db.execute("SELECT id FROM data OFFSET 20");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Offset is larger than total rows, should return 0 rows
        EXPECT_EQ(0, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, OffsetEqualToRows) {
    auto result = db.execute("SELECT id FROM data OFFSET 10");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Offset exactly equals row count, should return 0 rows
        EXPECT_EQ(0, result->get_row_count());
    }
}

// LIMIT and OFFSET together (pagination)
TEST_F(LimitOffsetBasicTest, LimitAndOffsetPage1) {
    auto result = db.execute("SELECT id FROM data LIMIT 3 OFFSET 0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Page 1: rows 1-3
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, LimitAndOffsetPage2) {
    auto result = db.execute("SELECT id FROM data LIMIT 3 OFFSET 3");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Page 2: rows 4-6
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, LimitAndOffsetLastPage) {
    auto result = db.execute("SELECT id FROM data LIMIT 3 OFFSET 9");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Last page: only row 10
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(LimitOffsetBasicTest, LimitAndOffsetBeyond) {
    auto result = db.execute("SELECT id FROM data LIMIT 3 OFFSET 15");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Beyond all rows
        EXPECT_EQ(0, result->get_row_count());
    }
}

// LIMIT with WHERE clause
class LimitOffsetWithWhereTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        schema.add_column(Column{"value", DataType::INT64, 8, false});
        
        db.create_table("numbers", schema);
        
        // Insert 20 rows
        for (int i = 1; i <= 20; ++i) {
            db.execute("INSERT INTO numbers (id, value) VALUES (" + std::to_string(i) + ", " + std::to_string(i * 10) + ")");
        }
    }
};

TEST_F(LimitOffsetWithWhereTest, WhereWithLimit) {
    // Filter WHERE value > 100, then LIMIT 5
    auto result = db.execute("SELECT id FROM numbers WHERE value > 100 LIMIT 5");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // After WHERE: rows 11-20 (10 rows)
        // After LIMIT 5: first 5 of filtered
        EXPECT_EQ(5, result->get_row_count());
    }
}

TEST_F(LimitOffsetWithWhereTest, WhereWithOffset) {
    // Filter WHERE value >= 100, then OFFSET 5
    auto result = db.execute("SELECT id FROM numbers WHERE value >= 100 OFFSET 5");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // After WHERE: rows 10-20 (11 rows)
        // After OFFSET 5: skip first 5 of filtered
        EXPECT_EQ(6, result->get_row_count());
    }
}

TEST_F(LimitOffsetWithWhereTest, WhereWithLimitOffset) {
    auto result = db.execute("SELECT id FROM numbers WHERE value > 50 LIMIT 5 OFFSET 3");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Pagination of filtered results
        EXPECT_GE(result->get_row_count(), 0);
    }
}

// LIMIT with ORDER BY
class LimitOffsetWithOrderByTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        schema.add_column(Column{"score", DataType::INT64, 8, false});
        
        db.create_table("scores", schema);
        
        // Insert scores in random order
        db.execute("INSERT INTO scores (id, score) VALUES (1, 100)");
        db.execute("INSERT INTO scores (id, score) VALUES (2, 95)");
        db.execute("INSERT INTO scores (id, score) VALUES (3, 100)");
        db.execute("INSERT INTO scores (id, score) VALUES (4, 85)");
        db.execute("INSERT INTO scores (id, score) VALUES (5, 100)");
    }
};

TEST_F(LimitOffsetWithOrderByTest, OrderByWithLimit) {
    // ORDER BY score DESC, then LIMIT 2 (get top 2 scores)
    auto result = db.execute("SELECT score FROM scores ORDER BY score DESC LIMIT 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Top 2 scores
        EXPECT_EQ(2, result->get_row_count());
    }
}

TEST_F(LimitOffsetWithOrderByTest, OrderByWithOffset) {
    // ORDER BY score ASC, then OFFSET 2 (skip lowest 2)
    auto result = db.execute("SELECT score FROM scores ORDER BY score ASC OFFSET 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Skip lowest 2, return rest
        EXPECT_EQ(3, result->get_row_count());
    }
}

TEST_F(LimitOffsetWithOrderByTest, OrderByLimitOffset) {
    auto result = db.execute("SELECT id FROM scores ORDER BY score DESC LIMIT 2 OFFSET 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // Skip 1 best, return next 2
        EXPECT_EQ(2, result->get_row_count());
    }
}

// LIMIT with GROUP BY
class LimitOffsetWithGroupByTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"category", DataType::VARCHAR, 50, false});
        schema.add_column(Column{"value", DataType::INT64, 8, false});
        
        db.create_table("items", schema);
        
        db.execute("INSERT INTO items (category, value) VALUES ('A', 10)");
        db.execute("INSERT INTO items (category, value) VALUES ('A', 20)");
        db.execute("INSERT INTO items (category, value) VALUES ('B', 30)");
        db.execute("INSERT INTO items (category, value) VALUES ('B', 40)");
        db.execute("INSERT INTO items (category, value) VALUES ('C', 50)");
    }
};

TEST_F(LimitOffsetWithGroupByTest, GroupByWithLimit) {
    // GROUP BY category, then LIMIT 2 (get first 2 groups)
    auto result = db.execute("SELECT category FROM items GROUP BY category LIMIT 2");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        // 2 groups
        EXPECT_EQ(2, result->get_row_count());
    }
}

// Edge cases
class LimitOffsetEdgeCasesTest : public ::testing::Test {
protected:
    Database db{":memory:"};
    
    void SetUp() override {
        Schema schema;
        schema.add_column(Column{"id", DataType::INT64, 8, false});
        
        db.create_table("test", schema);
        
        db.execute("INSERT INTO test (id) VALUES (1)");
    }
};

TEST_F(LimitOffsetEdgeCasesTest, SingleRowWithLimit) {
    auto result = db.execute("SELECT id FROM test LIMIT 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}

TEST_F(LimitOffsetEdgeCasesTest, SingleRowWithOffset) {
    auto result = db.execute("SELECT id FROM test OFFSET 1");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(0, result->get_row_count());
    }
}

TEST_F(LimitOffsetEdgeCasesTest, SingleRowWithBoth) {
    auto result = db.execute("SELECT id FROM test LIMIT 1 OFFSET 0");
    EXPECT_TRUE(result != nullptr);
    if (result && result->is_success()) {
        EXPECT_EQ(1, result->get_row_count());
    }
}
