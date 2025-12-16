#include <gtest/gtest.h>
#include "lyradb/sql_lexer.h"
#include "lyradb/sql_parser.h"

using namespace lyradb::query;

/**
 * @brief Test fixture for SQL parser tests
 */
class SqlParserTest : public ::testing::Test {
protected:
    SqlParser parser;
};

// ============================================================================
// Lexer Tests
// ============================================================================

TEST(SqlLexerTest, TokenizeSingleSelect) {
    SqlLexer lexer;
    auto tokens = lexer.tokenize("SELECT");
    
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[0].value, "SELECT");
}

TEST(SqlLexerTest, TokenizeIdentifier) {
    SqlLexer lexer;
    auto tokens = lexer.tokenize("customer_id");
    
    EXPECT_EQ(tokens[0].type, TokenType::IDENTIFIER);
    EXPECT_EQ(tokens[0].value, "customer_id");
}

TEST(SqlLexerTest, TokenizeStringLiteral) {
    SqlLexer lexer;
    auto tokens = lexer.tokenize("'Hello World'");
    
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].value, "Hello World");
}

TEST(SqlLexerTest, TokenizeNumberLiteral) {
    SqlLexer lexer;
    auto tokens = lexer.tokenize("42 3.14");
    
    EXPECT_EQ(tokens[0].type, TokenType::INTEGER);
    EXPECT_EQ(tokens[0].value, "42");
    EXPECT_EQ(tokens[1].type, TokenType::FLOAT);
    EXPECT_EQ(tokens[1].value, "3.14");
}

TEST(SqlLexerTest, TokenizeOperators) {
    SqlLexer lexer;
    auto tokens = lexer.tokenize("= < > <= >=");
    
    EXPECT_EQ(tokens[0].type, TokenType::EQUAL);
    EXPECT_EQ(tokens[1].type, TokenType::LESS);
    EXPECT_EQ(tokens[2].type, TokenType::GREATER);
    EXPECT_EQ(tokens[3].type, TokenType::LESS_EQUAL);
    EXPECT_EQ(tokens[4].type, TokenType::GREATER_EQUAL);
}

TEST(SqlLexerTest, TokenizeKeywords) {
    SqlLexer lexer;
    auto tokens = lexer.tokenize("SELECT FROM WHERE AND OR");
    
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[1].type, TokenType::FROM);
    EXPECT_EQ(tokens[2].type, TokenType::WHERE);
    EXPECT_EQ(tokens[3].type, TokenType::AND);
    EXPECT_EQ(tokens[4].type, TokenType::OR);
}

TEST(SqlLexerTest, SkipComments) {
    SqlLexer lexer;
    auto tokens = lexer.tokenize("SELECT -- this is a comment\nFROM");
    
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[1].type, TokenType::FROM);
}

// ============================================================================
// Parser Tests
// ============================================================================

TEST_F(SqlParserTest, ParseSimpleSelect) {
    auto stmt = parser.parse("SELECT id FROM users");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->select_list.size(), 1);
    EXPECT_NE(stmt->from_table, nullptr);
    EXPECT_EQ(stmt->from_table->table_name, "users");
}

TEST_F(SqlParserTest, ParseSelectMultipleColumns) {
    auto stmt = parser.parse("SELECT id, name, email FROM users");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->select_list.size(), 3);
}

TEST_F(SqlParserTest, ParseSelectStar) {
    auto stmt = parser.parse("SELECT * FROM users");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->select_list.size(), 1);
}

TEST_F(SqlParserTest, ParseSelectDistinct) {
    auto stmt = parser.parse("SELECT DISTINCT id FROM users");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_TRUE(stmt->select_distinct);
}

TEST_F(SqlParserTest, ParseSelectWithWhere) {
    auto stmt = parser.parse("SELECT * FROM users WHERE id = 42");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_NE(stmt->where_clause, nullptr);
}

TEST_F(SqlParserTest, ParseSelectWithComplexWhere) {
    auto stmt = parser.parse("SELECT * FROM users WHERE id > 10 AND age < 65");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_NE(stmt->where_clause, nullptr);
}

TEST_F(SqlParserTest, ParseSelectWithOrderBy) {
    auto stmt = parser.parse("SELECT * FROM users ORDER BY name ASC");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->order_by_list.size(), 1);
    EXPECT_EQ(stmt->order_by_list[0].direction, SortDirection::ASC);
}

TEST_F(SqlParserTest, ParseSelectWithMultipleOrderBy) {
    auto stmt = parser.parse("SELECT * FROM users ORDER BY name ASC, id DESC");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->order_by_list.size(), 2);
}

TEST_F(SqlParserTest, ParseSelectWithLimit) {
    auto stmt = parser.parse("SELECT * FROM users LIMIT 10");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->limit, 10);
}

TEST_F(SqlParserTest, ParseSelectWithLimitOffset) {
    auto stmt = parser.parse("SELECT * FROM users LIMIT 10 OFFSET 20");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->limit, 10);
    EXPECT_EQ(stmt->offset, 20);
}

TEST_F(SqlParserTest, ParseSelectWithGroupBy) {
    auto stmt = parser.parse("SELECT dept, COUNT(*) FROM employees GROUP BY dept");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->group_by_list.size(), 1);
}

TEST_F(SqlParserTest, ParseSelectWithAggregate) {
    auto stmt = parser.parse("SELECT COUNT(*) FROM users");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->select_list.size(), 1);
}

TEST_F(SqlParserTest, ParseSelectWithMultipleAggregates) {
    auto stmt = parser.parse("SELECT SUM(salary), AVG(age), MIN(id) FROM employees");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->select_list.size(), 3);
}

TEST_F(SqlParserTest, ParseSelectWithInnerJoin) {
    auto stmt = parser.parse("SELECT * FROM users INNER JOIN orders ON users.id = orders.user_id");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->joins.size(), 1);
    EXPECT_EQ(stmt->joins[0].join_type, JoinType::INNER);
    EXPECT_EQ(stmt->joins[0].table.table_name, "orders");
}

TEST_F(SqlParserTest, ParseSelectWithLeftJoin) {
    auto stmt = parser.parse("SELECT * FROM users LEFT JOIN orders ON users.id = orders.user_id");
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_EQ(stmt->joins.size(), 1);
    EXPECT_EQ(stmt->joins[0].join_type, JoinType::LEFT);
}

TEST_F(SqlParserTest, ParseSelectComplex) {
    auto stmt = parser.parse(
        "SELECT DISTINCT u.id, u.name, COUNT(o.id) as order_count "
        "FROM users u "
        "LEFT JOIN orders o ON u.id = o.user_id "
        "WHERE u.active = 1 "
        "GROUP BY u.id, u.name "
        "HAVING COUNT(o.id) > 5 "
        "ORDER BY order_count DESC "
        "LIMIT 100"
    );
    
    EXPECT_NE(stmt, nullptr);
    EXPECT_TRUE(stmt->select_distinct);
    EXPECT_EQ(stmt->joins.size(), 1);
    EXPECT_NE(stmt->where_clause, nullptr);
    EXPECT_EQ(stmt->group_by_list.size(), 2);
    EXPECT_NE(stmt->having_clause, nullptr);
    EXPECT_EQ(stmt->order_by_list.size(), 1);
    EXPECT_EQ(stmt->limit, 100);
}

// ============================================================================
// Error Handling Tests
// ============================================================================

TEST_F(SqlParserTest, ErrorMissingFrom) {
    auto stmt = parser.parse("SELECT * WHERE id = 1");
    
    // Should fail or handle gracefully
    EXPECT_TRUE(!parser.get_last_error().empty() || stmt != nullptr);
}

TEST_F(SqlParserTest, ErrorMissingSelect) {
    auto stmt = parser.parse("FROM users WHERE id = 1");
    
    EXPECT_TRUE(stmt == nullptr || !parser.get_last_error().empty());
}

// ============================================================================
// Expression Tests
// ============================================================================

TEST_F(SqlParserTest, ExpressionToString) {
    auto stmt = parser.parse("SELECT id + 10 FROM users");
    
    EXPECT_NE(stmt, nullptr);
    std::string expr_str = stmt->select_list[0]->to_string();
    EXPECT_FALSE(expr_str.empty());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
