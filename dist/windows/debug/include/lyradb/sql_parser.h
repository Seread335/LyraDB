#pragma once

#include <string>
#include <vector>
#include <memory>
#include <stdexcept>
#include "sql_lexer.h"

namespace lyradb {
namespace query {

// Forward declarations
class Expression;
class SelectStatement;
class CreateTableStatement;
class InsertStatement;
class TableReference;

/**
 * @brief Base class for all SQL statements
 */
class Statement {
public:
    virtual ~Statement() = default;
};

/**
 * @brief Binary operators
 */
enum class BinaryOp {
    ADD, SUBTRACT, MULTIPLY, DIVIDE, MODULO,
    EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    AND, OR, LIKE, IN
};

/**
 * @brief Unary operators
 */
enum class UnaryOp {
    NOT, NEGATE
};

/**
 * @brief Aggregate functions
 */
enum class AggregateFunc {
    COUNT, SUM, AVG, MIN, MAX
};

/**
 * @brief Sort direction
 */
enum class SortDirection {
    ASC, DESC
};

/**
 * @brief Join type
 */
enum class JoinType {
    INNER, LEFT, RIGHT, FULL
};

/**
 * @brief Base class for all expressions
 */
class Expression {
public:
    virtual ~Expression() = default;
    virtual std::string to_string() const = 0;
};

/**
 * @brief Literal value expression
 */
class LiteralExpr : public Expression {
public:
    explicit LiteralExpr(const Token& token);
    std::string to_string() const override;
    
    Token value;
};

/**
 * @brief Column reference expression
 */
class ColumnRefExpr : public Expression {
public:
    ColumnRefExpr(const std::string& col_name, const std::string& table_name = "")
        : column_name(col_name), table_name(table_name) {}
    
    std::string to_string() const override;
    
    std::string column_name;
    std::string table_name;  // Optional: for qualified column names
};

/**
 * @brief Binary operation expression
 */
class BinaryExpr : public Expression {
public:
    BinaryExpr(std::unique_ptr<Expression> left,
               BinaryOp op,
               std::unique_ptr<Expression> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}
    
    std::string to_string() const override;
    
    std::unique_ptr<Expression> left;
    BinaryOp op;
    std::unique_ptr<Expression> right;
};

/**
 * @brief Unary operation expression
 */
class UnaryExpr : public Expression {
public:
    UnaryExpr(UnaryOp op, std::unique_ptr<Expression> operand)
        : op(op), operand(std::move(operand)) {}
    
    std::string to_string() const override;
    
    UnaryOp op;
    std::unique_ptr<Expression> operand;
};

/**
 * @brief Function call expression
 */
class FunctionExpr : public Expression {
public:
    FunctionExpr(const std::string& name,
                 std::vector<std::unique_ptr<Expression>> args)
        : function_name(name), arguments(std::move(args)) {}
    
    std::string to_string() const override;
    
    std::string function_name;
    std::vector<std::unique_ptr<Expression>> arguments;
};

/**
 * @brief Aggregate function expression
 */
class AggregateExpr : public Expression {
public:
    AggregateExpr(AggregateFunc func, std::unique_ptr<Expression> arg = nullptr)
        : aggregate_func(func), argument(std::move(arg)) {}
    
    std::string to_string() const override;
    
    AggregateFunc aggregate_func;
    std::unique_ptr<Expression> argument;  // nullptr for COUNT(*)
};

/**
 * @brief Sort key specification
 */
struct SortKey {
    std::unique_ptr<Expression> expression;
    SortDirection direction;
    
    SortKey(std::unique_ptr<Expression> expr, SortDirection dir = SortDirection::ASC)
        : expression(std::move(expr)), direction(dir) {}
};

/**
 * @brief Table reference in FROM clause
 */
struct TableReference {
    std::string table_name;
    std::string alias;  // Optional alias
    
    TableReference(const std::string& name, const std::string& alias = "")
        : table_name(name), alias(alias) {}
};

/**
 * @brief JOIN specification
 */
struct JoinClause {
    JoinType join_type;
    TableReference table;
    std::unique_ptr<Expression> join_condition;
    
    JoinClause(JoinType type, const TableReference& t, 
               std::unique_ptr<Expression> cond = nullptr)
        : join_type(type), table(t), join_condition(std::move(cond)) {}
};

/**
 * @brief Column definition for CREATE TABLE
 */
struct ColumnDef {
    std::string column_name;
    std::string data_type;  // INT, BIGINT, FLOAT, DOUBLE, VARCHAR, BOOL
    bool nullable = true;
    
    ColumnDef(const std::string& name, const std::string& type, bool null = true)
        : column_name(name), data_type(type), nullable(null) {}
};

/**
 * @brief CREATE TABLE statement
 */
class CreateTableStatement : public Statement {
public:
    std::string table_name;
    std::vector<ColumnDef> columns;
    
    CreateTableStatement(const std::string& name) : table_name(name) {}
};

/**
 * @brief INSERT statement
 */
class InsertStatement : public Statement {
public:
    std::string table_name;
    std::vector<std::string> column_names;
    std::vector<std::vector<std::unique_ptr<Expression>>> values;  // rows of values
    
    InsertStatement(const std::string& name) : table_name(name) {}
};

/**
 * @brief UPDATE statement
 */
class UpdateStatement : public Statement {
public:
    std::string table_name;
    std::vector<std::pair<std::string, std::unique_ptr<Expression>>> assignments;  // column = value pairs
    std::unique_ptr<Expression> where_clause;
    
    UpdateStatement(const std::string& name) : table_name(name) {}
};

/**
 * @brief DELETE statement
 */
class DeleteStatement : public Statement {
public:
    std::string table_name;
    std::unique_ptr<Expression> where_clause;
    
    DeleteStatement(const std::string& name) : table_name(name) {}
};

/**
 * @brief CREATE INDEX statement
 */
class CreateIndexStatement : public Statement {
public:
    std::string index_name;
    std::string table_name;
    std::vector<std::string> columns;
    
    CreateIndexStatement(const std::string& name, const std::string& table)
        : index_name(name), table_name(table) {}
};

/**
 * @brief DROP statement (table or index)
 */
class DropStatement : public Statement {
public:
    enum Type { TABLE, INDEX };
    
    Type type;
    std::string object_name;
    bool if_exists;
    
    DropStatement(Type t, const std::string& name, bool exists = false)
        : type(t), object_name(name), if_exists(exists) {}
};

/**
 * @brief SELECT statement (complete query)
 */
class SelectStatement : public Statement {
public:
    // SELECT list
    std::vector<std::unique_ptr<Expression>> select_list;
    bool select_distinct = false;
    
    // FROM clause
    TableReference* from_table = nullptr;
    std::vector<JoinClause> joins;
    
    // WHERE clause
    std::unique_ptr<Expression> where_clause;
    
    // GROUP BY clause
    std::vector<std::unique_ptr<Expression>> group_by_list;
    std::unique_ptr<Expression> having_clause;
    
    // ORDER BY clause
    std::vector<SortKey> order_by_list;
    
    // LIMIT clause
    int64_t limit = -1;
    int64_t offset = 0;
    
    // Destructor
    ~SelectStatement();
};

/**
 * @brief SQL query parser (recursive descent)
 */
class SqlParser {
public:
    /**
     * @brief Parse SQL query string (returns base Statement pointer)
     * @param query SQL query text
     * @return Parsed statement (CreateTableStatement, InsertStatement, or SelectStatement)
     */
    std::unique_ptr<Statement> parse(const std::string& query);
    
    /**
     * @brief Parse SELECT statement specifically
     * @param query SQL SELECT query text
     * @return Parsed SelectStatement
     */
    std::unique_ptr<SelectStatement> parse_select_statement(const std::string& query);
    
    /**
     * @brief Get last error message
     * @return Error description
     */
    const std::string& get_last_error() const { return last_error_; }
    
    /**
     * @brief Get detailed error information with line and column
     * @return Formatted error message with context
     */
    const std::string& get_detailed_error() const { return detailed_error_; }

private:
    std::vector<Token> tokens_;
    size_t current_token_;
    std::string last_error_;
    std::string detailed_error_;
    
    // Navigation
    const Token& current() const;
    const Token& peek(size_t offset = 1) const;
    bool match(TokenType type);
    bool check(TokenType type) const;
    void advance();
    const Token& consume(TokenType type, const std::string& message);
    
    // Parsing methods - Statements
    std::unique_ptr<SelectStatement> parse_select();
    std::unique_ptr<CreateTableStatement> parse_create_table();
    std::unique_ptr<InsertStatement> parse_insert();
    std::unique_ptr<UpdateStatement> parse_update();
    std::unique_ptr<DeleteStatement> parse_delete();
    std::unique_ptr<CreateIndexStatement> parse_create_index();
    std::unique_ptr<DropStatement> parse_drop();
    
    // Parsing methods - Select specific
    void parse_select_list(SelectStatement* stmt);
    void parse_from_clause(SelectStatement* stmt);
    void parse_join_clauses(SelectStatement* stmt);
    void parse_where_clause(SelectStatement* stmt);
    void parse_group_by_clause(SelectStatement* stmt);
    void parse_having_clause(SelectStatement* stmt);
    void parse_order_by_clause(SelectStatement* stmt);
    void parse_limit_clause(SelectStatement* stmt);
    
    // Expression parsing (operator precedence)
    std::unique_ptr<Expression> parse_expression();
    std::unique_ptr<Expression> parse_or_expression();
    std::unique_ptr<Expression> parse_and_expression();
    std::unique_ptr<Expression> parse_comparison_expression();
    std::unique_ptr<Expression> parse_additive_expression();
    std::unique_ptr<Expression> parse_multiplicative_expression();
    std::unique_ptr<Expression> parse_unary_expression();
    std::unique_ptr<Expression> parse_primary_expression();
    
    // Utility
    void error(const std::string& message);
    void detailed_error(const std::string& message, const Token* context_token = nullptr);
};

}  // namespace query
}  // namespace lyradb
