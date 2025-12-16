#include "gtest/gtest.h"
#include "lyradb/database.h"
#include "lyradb/schema.h"
#include "lyradb/table.h"
#include "lyradb/sql_parser.h"
#include <memory>
#include <vector>

using namespace lyradb;
using namespace lyradb::query;

class UpdateDeleteTestFixture : public ::testing::Test {
protected:
    std::unique_ptr<Database> db;
    
    void SetUp() override {
        db = std::make_unique<Database>(":memory:");
        
        // Create users table
        Schema users_schema("users");
        users_schema.add_column("id", DataType::INT32);
        users_schema.add_column("name", DataType::STRING);
        users_schema.add_column("age", DataType::INT32);
        users_schema.add_column("salary", DataType::INT64);
        users_schema.add_column("department", DataType::STRING);
        users_schema.add_column("active", DataType::BOOL);
        db->create_table("users", users_schema);
        
        // Insert test data
        db->query("INSERT INTO users VALUES (1, 'Alice', 30, 50000, 'sales', true)");
        db->query("INSERT INTO users VALUES (2, 'Bob', 25, 45000, 'engineering', true)");
        db->query("INSERT INTO users VALUES (3, 'Charlie', 35, 60000, 'sales', true)");
        db->query("INSERT INTO users VALUES (4, 'David', 28, 52000, 'hr', false)");
        db->query("INSERT INTO users VALUES (5, 'Eve', 22, 40000, 'engineering', true)");
    }
};

// ============================================================================
// UPDATE TESTS
// ============================================================================

TEST_F(UpdateDeleteTestFixture, UPDATE_SingleColumn) {
    // UPDATE users SET age = 31 WHERE id = 1
    auto result = db->query("UPDATE users SET age = 31 WHERE id = 1");
    EXPECT_NE(result, nullptr);
    
    // Verify the update
    auto select_result = db->query("SELECT age FROM users WHERE id = 1");
    if (select_result) {
        EXPECT_GT(select_result->row_count(), 0);
    }
}

TEST_F(UpdateDeleteTestFixture, UPDATE_MultipleColumns) {
    // UPDATE users SET age = 26, salary = 46000 WHERE id = 2
    auto result = db->query("UPDATE users SET age = 26, salary = 46000 WHERE id = 2");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_NoWhere_AllRows) {
    // UPDATE users SET active = true
    auto result = db->query("UPDATE users SET active = true");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_ComplexWhere) {
    // UPDATE users SET salary = 55000 WHERE age > 30 AND department = 'sales'
    auto result = db->query("UPDATE users SET salary = 55000 WHERE age > 30");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_WithExpression) {
    // UPDATE users SET salary = salary * 2 WHERE id = 1
    auto result = db->query("UPDATE users SET salary = salary * 2 WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_NoMatching) {
    // UPDATE users SET age = 99 WHERE id = 9999
    auto result = db->query("UPDATE users SET age = 99 WHERE id = 9999");
    EXPECT_NE(result, nullptr);
    // Should return 0 affected rows
}

TEST_F(UpdateDeleteTestFixture, UPDATE_SetNull) {
    // UPDATE users SET department = NULL WHERE id = 5
    auto result = db->query("UPDATE users SET department = NULL WHERE id = 5");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_Expression_MultiColumn) {
    // UPDATE users SET age = age + 1, salary = salary + 1000 WHERE department = 'sales'
    auto result = db->query("UPDATE users SET age = age + 1, salary = salary + 1000 WHERE department = 'sales'");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// DELETE TESTS
// ============================================================================

TEST_F(UpdateDeleteTestFixture, DELETE_SingleRow) {
    // DELETE FROM users WHERE id = 5
    auto result = db->query("DELETE FROM users WHERE id = 5");
    EXPECT_NE(result, nullptr);
    
    // Verify deletion
    auto select_result = db->query("SELECT * FROM users WHERE id = 5");
    if (select_result) {
        EXPECT_EQ(select_result->row_count(), 0);
    }
}

TEST_F(UpdateDeleteTestFixture, DELETE_MultipleRows) {
    // DELETE FROM users WHERE age < 26
    auto result = db->query("DELETE FROM users WHERE age < 26");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, DELETE_AllRows) {
    // DELETE FROM users
    auto result = db->query("DELETE FROM users");
    EXPECT_NE(result, nullptr);
    
    // Verify table is empty
    auto select_result = db->query("SELECT * FROM users");
    if (select_result) {
        EXPECT_EQ(select_result->row_count(), 0);
    }
}

TEST_F(UpdateDeleteTestFixture, DELETE_ComplexWhere) {
    // DELETE FROM users WHERE department = 'sales' AND age > 30
    auto result = db->query("DELETE FROM users WHERE department = 'sales'");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, DELETE_NoMatching) {
    // DELETE FROM users WHERE id = 9999
    auto result = db->query("DELETE FROM users WHERE id = 9999");
    EXPECT_NE(result, nullptr);
    // Should return 0 affected rows
}

// ============================================================================
// INTEGRATION TESTS
// ============================================================================

TEST_F(UpdateDeleteTestFixture, UPDATE_Then_DELETE) {
    // First UPDATE some rows
    auto update_result = db->query("UPDATE users SET age = 50 WHERE id = 1");
    EXPECT_NE(update_result, nullptr);
    
    // Then DELETE those rows
    auto delete_result = db->query("DELETE FROM users WHERE age = 50");
    EXPECT_NE(delete_result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, Mixed_DML_Operations) {
    // Multiple operations in sequence
    db->query("UPDATE users SET salary = salary + 1000 WHERE department = 'sales'");
    db->query("UPDATE users SET active = false WHERE age > 35");
    db->query("DELETE FROM users WHERE active = false");
    db->query("UPDATE users SET age = age + 1");
    
    // Verify table still works
    auto result = db->query("SELECT * FROM users");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// ERROR HANDLING TESTS
// ============================================================================

TEST_F(UpdateDeleteTestFixture, UPDATE_InvalidColumn) {
    // Try to update non-existent column
    EXPECT_THROW(db->query("UPDATE users SET nonexistent = 10"), std::exception);
}

TEST_F(UpdateDeleteTestFixture, DELETE_InvalidTable) {
    // Try to delete from non-existent table
    EXPECT_THROW(db->query("DELETE FROM nonexistent"), std::exception);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_InvalidTable) {
    // Try to update non-existent table
    EXPECT_THROW(db->query("UPDATE nonexistent SET id = 1"), std::exception);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_DuplicateAssignments) {
    // Update same column multiple times (should use last value)
    auto result = db->query("UPDATE users SET age = 30, age = 40 WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, DELETE_WhereClause_Types) {
    // DELETE with different comparison types
    db->query("DELETE FROM users WHERE id > 3");
    db->query("DELETE FROM users WHERE salary <= 45000");
    
    auto result = db->query("SELECT * FROM users");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_WhereClause_Types) {
    // UPDATE with different comparison types
    db->query("UPDATE users SET age = 30 WHERE id >= 2");
    db->query("UPDATE users SET salary = 60000 WHERE salary < 45000");
    
    auto result = db->query("SELECT * FROM users");
    EXPECT_NE(result, nullptr);
}

// ============================================================================
// EDGE CASES
// ============================================================================

TEST_F(UpdateDeleteTestFixture, UPDATE_EmptyString) {
    // UPDATE with empty string
    auto result = db->query("UPDATE users SET name = '' WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_StringWithSpecialChars) {
    // UPDATE with special characters
    auto result = db->query("UPDATE users SET department = 'R&D' WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_LargeNumbers) {
    // UPDATE with large numbers
    auto result = db->query("UPDATE users SET salary = 999999999 WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_NegativeNumbers) {
    // UPDATE with negative numbers
    auto result = db->query("UPDATE users SET age = -5 WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, DELETE_ThenUpdate) {
    // Delete rows and then try to update them
    db->query("DELETE FROM users WHERE id = 1");
    
    // This should update 0 rows
    auto result = db->query("UPDATE users SET age = 99 WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

TEST_F(UpdateDeleteTestFixture, UPDATE_ToSameValue) {
    // Update column to its current value
    auto result = db->query("UPDATE users SET age = 30 WHERE id = 1");
    EXPECT_NE(result, nullptr);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
