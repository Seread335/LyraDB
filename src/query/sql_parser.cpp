#include "lyradb/sql_parser.h"
#include <sstream>
#include <algorithm>

namespace lyradb {
namespace query {

// Expression implementations
LiteralExpr::LiteralExpr(const Token& token) : value(token) {}

std::string LiteralExpr::to_string() const {
    return value.value;
}

std::string ColumnRefExpr::to_string() const {
    if (table_name.empty()) {
        return column_name;
    }
    return table_name + "." + column_name;
}

std::string BinaryExpr::to_string() const {
    std::string op_str;
    switch (op) {
        case BinaryOp::ADD: op_str = "+"; break;
        case BinaryOp::SUBTRACT: op_str = "-"; break;
        case BinaryOp::MULTIPLY: op_str = "*"; break;
        case BinaryOp::DIVIDE: op_str = "/"; break;
        case BinaryOp::MODULO: op_str = "%"; break;
        case BinaryOp::EQUAL: op_str = "="; break;
        case BinaryOp::NOT_EQUAL: op_str = "!="; break;
        case BinaryOp::LESS: op_str = "<"; break;
        case BinaryOp::GREATER: op_str = ">"; break;
        case BinaryOp::LESS_EQUAL: op_str = "<="; break;
        case BinaryOp::GREATER_EQUAL: op_str = ">="; break;
        case BinaryOp::AND: op_str = "AND"; break;
        case BinaryOp::OR: op_str = "OR"; break;
        case BinaryOp::LIKE: op_str = "LIKE"; break;
        case BinaryOp::IN: op_str = "IN"; break;
    }
    return "(" + left->to_string() + " " + op_str + " " + right->to_string() + ")";
}

std::string UnaryExpr::to_string() const {
    std::string op_str = (op == UnaryOp::NOT) ? "NOT " : "-";
    return op_str + operand->to_string();
}

std::string FunctionExpr::to_string() const {
    std::string result = function_name + "(";
    for (size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) result += ", ";
        result += arguments[i]->to_string();
    }
    result += ")";
    return result;
}

std::string AggregateExpr::to_string() const {
    std::string func_name;
    switch (aggregate_func) {
        case AggregateFunc::COUNT: func_name = "COUNT"; break;
        case AggregateFunc::SUM: func_name = "SUM"; break;
        case AggregateFunc::AVG: func_name = "AVG"; break;
        case AggregateFunc::MIN: func_name = "MIN"; break;
        case AggregateFunc::MAX: func_name = "MAX"; break;
    }
    
    if (argument) {
        return func_name + "(" + argument->to_string() + ")";
    }
    return func_name + "(*)";
}

SelectStatement::~SelectStatement() {
    delete from_table;
}

// SqlParser implementation

std::unique_ptr<Statement> SqlParser::parse(const std::string& query) {
    SqlLexer lexer;
    tokens_ = lexer.tokenize(query);
    current_token_ = 0;
    last_error_ = "";
    
    try {
        // Check first token to determine statement type
        if (check(TokenType::CREATE)) {
            // Need to peek at second token to distinguish CREATE TABLE vs CREATE INDEX
            advance();  // consume CREATE
            if (check(TokenType::INDEX)) {
                current_token_ = 0;  // Reset for parse_create_index to handle from start
                return parse_create_index();
            } else {
                current_token_ = 0;  // Reset for parse_create_table to handle from start
                return parse_create_table();
            }
        } else if (check(TokenType::INSERT)) {
            return parse_insert();
        } else if (check(TokenType::UPDATE)) {
            return parse_update();
        } else if (check(TokenType::DELETE)) {
            return parse_delete();
        } else if (check(TokenType::DROP)) {
            return parse_drop();
        } else if (check(TokenType::SELECT)) {
            return parse_select();
        } else {
            error("Unexpected token: expected CREATE, INSERT, UPDATE, DELETE, DROP, or SELECT");
            return nullptr;
        }
    } catch (const std::exception& e) {
        last_error_ = e.what();
        return nullptr;
    }
}

std::unique_ptr<SelectStatement> SqlParser::parse_select_statement(const std::string& query) {
    SqlLexer lexer;
    tokens_ = lexer.tokenize(query);
    current_token_ = 0;
    last_error_ = "";
    
    try {
        return parse_select();
    } catch (const std::exception& e) {
        last_error_ = e.what();
        return nullptr;
    }
}

const Token& SqlParser::current() const {
    if (current_token_ >= tokens_.size()) {
        static Token eof_token(TokenType::END_OF_INPUT, "", 0, 0);
        return eof_token;
    }
    return tokens_[current_token_];
}

const Token& SqlParser::peek(size_t offset) const {
    size_t pos = current_token_ + offset;
    if (pos >= tokens_.size()) {
        static Token eof_token(TokenType::END_OF_INPUT, "", 0, 0);
        return eof_token;
    }
    return tokens_[pos];
}

bool SqlParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool SqlParser::check(TokenType type) const {
    return current().type == type;
}

void SqlParser::advance() {
    if (current_token_ < tokens_.size()) {
        current_token_++;
    }
}

const Token& SqlParser::consume(TokenType type, const std::string& message) {
    if (check(type)) {
        const Token& token = current();
        advance();
        return token;
    }
    error(message);
    throw std::runtime_error(message);
}

std::unique_ptr<SelectStatement> SqlParser::parse_select() {
    consume(TokenType::SELECT, "Expected SELECT");
    
    auto stmt = std::make_unique<SelectStatement>();
    
    // Check for DISTINCT
    if (match(TokenType::DISTINCT)) {
        stmt->select_distinct = true;
    }
    
    parse_select_list(stmt.get());
    
    if (match(TokenType::FROM)) {
        parse_from_clause(stmt.get());
        
        // Parse JOINs
        while (check(TokenType::JOIN) || check(TokenType::INNER) || 
               check(TokenType::LEFT) || check(TokenType::RIGHT)) {
            parse_join_clauses(stmt.get());
        }
    }
    
    if (match(TokenType::WHERE)) {
        parse_where_clause(stmt.get());
    }
    
    if (match(TokenType::GROUP)) {
        consume(TokenType::BY, "Expected BY after GROUP");
        parse_group_by_clause(stmt.get());
    }
    
    if (match(TokenType::HAVING)) {
        parse_having_clause(stmt.get());
    }
    
    if (match(TokenType::ORDER)) {
        consume(TokenType::BY, "Expected BY after ORDER");
        parse_order_by_clause(stmt.get());
    }
    
    if (match(TokenType::LIMIT)) {
        parse_limit_clause(stmt.get());
    }
    
    return stmt;
}

void SqlParser::parse_select_list(SelectStatement* stmt) {
    if (match(TokenType::STAR)) {
        // SELECT *
        auto col_ref = std::make_unique<ColumnRefExpr>("*");
        stmt->select_list.push_back(std::move(col_ref));
    } else {
        do {
            auto expr = parse_expression();
            
            // Handle alias (AS optional_alias_name)
            if (match(TokenType::AS)) {
                // For now, just consume the alias
                consume(TokenType::IDENTIFIER, "Expected alias name");
            }
            
            stmt->select_list.push_back(std::move(expr));
        } while (match(TokenType::COMMA));
    }
}

void SqlParser::parse_from_clause(SelectStatement* stmt) {
    Token table_name = consume(TokenType::IDENTIFIER, "Expected table name");
    
    std::string alias;
    if (match(TokenType::AS)) {
        alias = consume(TokenType::IDENTIFIER, "Expected alias").value;
    } else if (check(TokenType::IDENTIFIER)) {
        // Implicit alias
        alias = current().value;
        advance();
    }
    
    stmt->from_table = new TableReference(table_name.value, alias);
}

void SqlParser::parse_join_clauses(SelectStatement* stmt) {
    JoinType join_type = JoinType::INNER;
    
    if (match(TokenType::LEFT)) {
        join_type = JoinType::LEFT;
    } else if (match(TokenType::RIGHT)) {
        join_type = JoinType::RIGHT;
    } else if (match(TokenType::FULL)) {
        join_type = JoinType::FULL;
    } else if (match(TokenType::INNER)) {
        join_type = JoinType::INNER;
    }
    
    consume(TokenType::JOIN, "Expected JOIN");
    
    Token table_name = consume(TokenType::IDENTIFIER, "Expected table name");
    std::string alias;
    if (match(TokenType::AS)) {
        alias = consume(TokenType::IDENTIFIER, "Expected alias").value;
    }
    
    consume(TokenType::ON, "Expected ON in JOIN");
    auto join_condition = parse_expression();
    
    stmt->joins.emplace_back(join_type, TableReference(table_name.value, alias),
                            std::move(join_condition));
}

void SqlParser::parse_where_clause(SelectStatement* stmt) {
    stmt->where_clause = parse_expression();
}

void SqlParser::parse_group_by_clause(SelectStatement* stmt) {
    do {
        stmt->group_by_list.push_back(parse_expression());
    } while (match(TokenType::COMMA));
}

void SqlParser::parse_having_clause(SelectStatement* stmt) {
    stmt->having_clause = parse_expression();
}

void SqlParser::parse_order_by_clause(SelectStatement* stmt) {
    do {
        auto expr = parse_expression();
        SortDirection dir = SortDirection::ASC;
        
        if (match(TokenType::DESC)) {
            dir = SortDirection::DESC;
        } else {
            match(TokenType::ASC);  // Optional ASC
        }
        
        stmt->order_by_list.emplace_back(std::move(expr), dir);
    } while (match(TokenType::COMMA));
}

void SqlParser::parse_limit_clause(SelectStatement* stmt) {
    Token limit_token = consume(TokenType::INTEGER, "Expected limit value");
    stmt->limit = std::stoll(limit_token.value);
    
    if (match(TokenType::OFFSET)) {
        Token offset_token = consume(TokenType::INTEGER, "Expected offset value");
        stmt->offset = std::stoll(offset_token.value);
    }
}

std::unique_ptr<CreateTableStatement> SqlParser::parse_create_table() {
    consume(TokenType::CREATE, "Expected CREATE");
    consume(TokenType::TABLE, "Expected TABLE");
    
    Token table_name_token = consume(TokenType::IDENTIFIER, "Expected table name");
    auto stmt = std::make_unique<CreateTableStatement>(table_name_token.value);
    
    consume(TokenType::LPAREN, "Expected ( after table name");
    
    // Parse column definitions
    do {
        Token col_name = consume(TokenType::IDENTIFIER, "Expected column name");
        
        // Parse data type
        std::string data_type;
        if (check(TokenType::INT)) {
            advance();
            data_type = "INT";
        } else if (check(TokenType::BIGINT)) {
            advance();
            data_type = "BIGINT";
        } else if (check(TokenType::FLOAT_TYPE)) {
            advance();
            data_type = "FLOAT";
        } else if (check(TokenType::DOUBLE)) {
            advance();
            data_type = "DOUBLE";
        } else if (check(TokenType::VARCHAR)) {
            advance();
            data_type = "VARCHAR";
            // VARCHAR(n) - consume the size if present
            if (match(TokenType::LPAREN)) {
                consume(TokenType::INTEGER, "Expected integer size");
                consume(TokenType::RPAREN, "Expected )");
            }
        } else if (check(TokenType::BOOL_TYPE)) {
            advance();
            data_type = "BOOL";
        } else {
            error("Expected data type");
            throw std::runtime_error("Expected data type");
        }
        
        stmt->columns.emplace_back(col_name.value, data_type);
    } while (match(TokenType::COMMA));
    
    consume(TokenType::RPAREN, "Expected ) after column definitions");
    match(TokenType::SEMICOLON);  // Optional semicolon
    
    return stmt;
}

std::unique_ptr<InsertStatement> SqlParser::parse_insert() {
    consume(TokenType::INSERT, "Expected INSERT");
    consume(TokenType::INTO, "Expected INTO");
    
    Token table_name_token = consume(TokenType::IDENTIFIER, "Expected table name");
    auto stmt = std::make_unique<InsertStatement>(table_name_token.value);
    
    // Optional column list: (col1, col2, ...)
    if (match(TokenType::LPAREN)) {
        do {
            Token col_name = consume(TokenType::IDENTIFIER, "Expected column name");
            stmt->column_names.push_back(col_name.value);
        } while (match(TokenType::COMMA));
        
        consume(TokenType::RPAREN, "Expected ) after column list");
    }
    
    consume(TokenType::VALUES, "Expected VALUES");
    
    // Parse value lists: (val1, val2), (val3, val4), ...
    do {
        consume(TokenType::LPAREN, "Expected ( before values");
        std::vector<std::unique_ptr<Expression>> row_values;
        
        do {
            row_values.push_back(parse_expression());
        } while (match(TokenType::COMMA));
        
        consume(TokenType::RPAREN, "Expected ) after values");
        stmt->values.push_back(std::move(row_values));
    } while (match(TokenType::COMMA));
    
    match(TokenType::SEMICOLON);  // Optional semicolon
    
    return stmt;
}

std::unique_ptr<UpdateStatement> SqlParser::parse_update() {
    consume(TokenType::UPDATE, "Expected UPDATE");
    
    Token table_name_token = consume(TokenType::IDENTIFIER, "Expected table name");
    auto stmt = std::make_unique<UpdateStatement>(table_name_token.value);
    
    consume(TokenType::SET, "Expected SET");
    
    // Parse assignments: col1 = val1, col2 = val2, ...
    do {
        Token col_name = consume(TokenType::IDENTIFIER, "Expected column name");
        consume(TokenType::EQUAL, "Expected = in SET clause");
        auto expr = parse_expression();
        stmt->assignments.push_back({col_name.value, std::move(expr)});
    } while (match(TokenType::COMMA));
    
    // Parse optional WHERE clause
    if (check(TokenType::WHERE)) {
        advance();  // consume WHERE
        stmt->where_clause = parse_expression();
    }
    
    match(TokenType::SEMICOLON);  // Optional semicolon
    
    return stmt;
}

std::unique_ptr<DeleteStatement> SqlParser::parse_delete() {
    consume(TokenType::DELETE, "Expected DELETE");
    consume(TokenType::FROM, "Expected FROM");
    
    Token table_name_token = consume(TokenType::IDENTIFIER, "Expected table name");
    auto stmt = std::make_unique<DeleteStatement>(table_name_token.value);
    
    // Parse optional WHERE clause
    if (check(TokenType::WHERE)) {
        advance();  // consume WHERE
        stmt->where_clause = parse_expression();
    }
    
    match(TokenType::SEMICOLON);  // Optional semicolon
    
    return stmt;
}

std::unique_ptr<CreateIndexStatement> SqlParser::parse_create_index() {
    consume(TokenType::CREATE, "Expected CREATE");
    consume(TokenType::INDEX, "Expected INDEX");
    
    Token index_name = consume(TokenType::IDENTIFIER, "Expected index name");
    consume(TokenType::ON, "Expected ON");
    Token table_name = consume(TokenType::IDENTIFIER, "Expected table name");
    
    auto stmt = std::make_unique<CreateIndexStatement>(index_name.value, table_name.value);
    
    // Parse column list: (col1, col2, ...)
    consume(TokenType::LPAREN, "Expected ( before columns");
    do {
        Token col_name = consume(TokenType::IDENTIFIER, "Expected column name");
        stmt->columns.push_back(col_name.value);
    } while (match(TokenType::COMMA));
    
    consume(TokenType::RPAREN, "Expected ) after columns");
    match(TokenType::SEMICOLON);  // Optional semicolon
    
    return stmt;
}

std::unique_ptr<DropStatement> SqlParser::parse_drop() {
    consume(TokenType::DROP, "Expected DROP");
    
    bool is_index = false;
    if (match(TokenType::INDEX)) {
        is_index = true;
    } else {
        consume(TokenType::TABLE, "Expected TABLE or INDEX");
    }
    
    bool if_exists = false;
    if (match(TokenType::IF)) {
        consume(TokenType::EXISTS, "Expected EXISTS");
        if_exists = true;
    }
    
    Token object_name = consume(TokenType::IDENTIFIER, "Expected object name");
    
    auto stmt = std::make_unique<DropStatement>(
        is_index ? DropStatement::INDEX : DropStatement::TABLE,
        object_name.value,
        if_exists
    );
    
    match(TokenType::SEMICOLON);  // Optional semicolon
    
    return stmt;
}

std::unique_ptr<Expression> SqlParser::parse_expression() {
    return parse_or_expression();
}

std::unique_ptr<Expression> SqlParser::parse_or_expression() {
    auto left = parse_and_expression();
    
    while (match(TokenType::OR)) {
        auto right = parse_and_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::OR, std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> SqlParser::parse_and_expression() {
    auto left = parse_comparison_expression();
    
    while (match(TokenType::AND)) {
        auto right = parse_comparison_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::AND, std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> SqlParser::parse_comparison_expression() {
    auto left = parse_additive_expression();
    
    if (match(TokenType::EQUAL)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::EQUAL, std::move(right));
    } else if (match(TokenType::NOT_EQUAL)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::NOT_EQUAL, std::move(right));
    } else if (match(TokenType::LESS)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::LESS, std::move(right));
    } else if (match(TokenType::GREATER)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::GREATER, std::move(right));
    } else if (match(TokenType::LESS_EQUAL)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::LESS_EQUAL, std::move(right));
    } else if (match(TokenType::GREATER_EQUAL)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::GREATER_EQUAL, std::move(right));
    } else if (match(TokenType::LIKE)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::LIKE, std::move(right));
    } else if (match(TokenType::IN)) {
        auto right = parse_additive_expression();
        left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::IN, std::move(right));
    }
    
    return left;
}

std::unique_ptr<Expression> SqlParser::parse_additive_expression() {
    auto left = parse_multiplicative_expression();
    
    while (true) {
        if (match(TokenType::PLUS)) {
            auto right = parse_multiplicative_expression();
            left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::ADD, std::move(right));
        } else if (match(TokenType::MINUS)) {
            auto right = parse_multiplicative_expression();
            left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::SUBTRACT, std::move(right));
        } else {
            break;
        }
    }
    
    return left;
}

std::unique_ptr<Expression> SqlParser::parse_multiplicative_expression() {
    auto left = parse_unary_expression();
    
    while (true) {
        if (match(TokenType::MULTIPLY)) {
            auto right = parse_unary_expression();
            left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::MULTIPLY, std::move(right));
        } else if (match(TokenType::DIVIDE)) {
            auto right = parse_unary_expression();
            left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::DIVIDE, std::move(right));
        } else if (match(TokenType::MODULO)) {
            auto right = parse_unary_expression();
            left = std::make_unique<BinaryExpr>(std::move(left), BinaryOp::MODULO, std::move(right));
        } else {
            break;
        }
    }
    
    return left;
}

std::unique_ptr<Expression> SqlParser::parse_unary_expression() {
    if (match(TokenType::NOT)) {
        auto operand = parse_unary_expression();
        return std::make_unique<UnaryExpr>(UnaryOp::NOT, std::move(operand));
    } else if (match(TokenType::MINUS)) {
        auto operand = parse_unary_expression();
        return std::make_unique<UnaryExpr>(UnaryOp::NEGATE, std::move(operand));
    }
    
    return parse_primary_expression();
}

std::unique_ptr<Expression> SqlParser::parse_primary_expression() {
    // Parenthesized expression
    if (match(TokenType::LPAREN)) {
        // Check if this is a SELECT subquery
        if (check(TokenType::SELECT)) {
            error("Subqueries not yet supported");
            throw std::runtime_error("Subqueries not yet supported");
        }
        
        auto expr = parse_expression();
        consume(TokenType::RPAREN, "Expected )");
        return expr;
    }
    
    // NULL literal
    if (match(TokenType::NULL_KW)) {
        return std::make_unique<LiteralExpr>(Token(TokenType::NULL_KW, "NULL"));
    }
    
    // String literal
    if (check(TokenType::STRING)) {
        auto token = current();
        advance();
        return std::make_unique<LiteralExpr>(token);
    }
    
    // Numeric literal
    if (check(TokenType::INTEGER) || check(TokenType::FLOAT)) {
        auto token = current();
        advance();
        return std::make_unique<LiteralExpr>(token);
    }
    
    // Aggregate or function call
    if (check(TokenType::COUNT) || check(TokenType::SUM) || 
        check(TokenType::AVG) || check(TokenType::MIN) || check(TokenType::MAX)) {
        
        Token agg_token = current();
        advance();
        
        AggregateFunc func;
        if (agg_token.type == TokenType::COUNT) func = AggregateFunc::COUNT;
        else if (agg_token.type == TokenType::SUM) func = AggregateFunc::SUM;
        else if (agg_token.type == TokenType::AVG) func = AggregateFunc::AVG;
        else if (agg_token.type == TokenType::MIN) func = AggregateFunc::MIN;
        else func = AggregateFunc::MAX;
        
        consume(TokenType::LPAREN, "Expected ( in aggregate function");
        
        std::unique_ptr<Expression> arg;
        if (match(TokenType::STAR)) {
            // COUNT(*)
            arg = nullptr;
        } else {
            arg = parse_expression();
        }
        
        consume(TokenType::RPAREN, "Expected )");
        
        return std::make_unique<AggregateExpr>(func, std::move(arg));
    }
    
    // Identifier (column reference or function call)
    if (check(TokenType::IDENTIFIER)) {
        Token id_token = current();
        advance();
        
        // Check for function call
        if (match(TokenType::LPAREN)) {
            std::vector<std::unique_ptr<Expression>> args;
            
            if (!check(TokenType::RPAREN)) {
                do {
                    args.push_back(parse_expression());
                } while (match(TokenType::COMMA));
            }
            
            consume(TokenType::RPAREN, "Expected )");
            return std::make_unique<FunctionExpr>(id_token.value, std::move(args));
        }
        
        // Column reference with optional table qualifier
        std::string table_name;
        std::string column_name = id_token.value;
        
        if (match(TokenType::DOT)) {
            table_name = column_name;
            column_name = consume(TokenType::IDENTIFIER, "Expected column name").value;
        }
        
        return std::make_unique<ColumnRefExpr>(column_name, table_name);
    }
    
    error("Unexpected token in expression");
    throw std::runtime_error("Unexpected token in expression");
}

void SqlParser::error(const std::string& message) {
    const Token& tok = current();
    last_error_ = message + " at line " + std::to_string(tok.line) + 
                  ", column " + std::to_string(tok.column) +
                  " (token: '" + tok.value + "')";
    
    // Also build detailed error with context
    detailed_error_ = "SQL Syntax Error:\n";
    detailed_error_ += "  Message: " + message + "\n";
    detailed_error_ += "  Location: Line " + std::to_string(tok.line) + 
                       ", Column " + std::to_string(tok.column) + "\n";
    detailed_error_ += "  Token: '" + tok.value + "'\n";
    
    // Suggest common fixes
    if (message.find("Expected") != std::string::npos) {
        detailed_error_ += "  Hint: Check your SQL syntax near this token\n";
    }
    if (message.find("table") != std::string::npos) {
        detailed_error_ += "  Hint: Verify table name and CREATE TABLE statement\n";
    }
    if (message.find("column") != std::string::npos) {
        detailed_error_ += "  Hint: Check column name spelling and data type\n";
    }
}

void SqlParser::detailed_error(const std::string& message, const Token* context_token) {
    const Token& tok = context_token ? *context_token : current();
    detailed_error_ = "SQL Syntax Error:\n";
    detailed_error_ += "  Message: " + message + "\n";
    detailed_error_ += "  Location: Line " + std::to_string(tok.line) + 
                       ", Column " + std::to_string(tok.column) + "\n";
    detailed_error_ += "  Token: '" + tok.value + "'\n";
    detailed_error_ += "  Expected: Check SQL documentation for proper syntax\n";
    
    last_error_ = message;
}

}  // namespace query
}  // namespace lyradb
