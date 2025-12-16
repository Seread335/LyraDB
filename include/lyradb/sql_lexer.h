#pragma once

#include <string>
#include <vector>
#include <cstdint>

namespace lyradb {
namespace query {

/**
 * @brief SQL token types
 */
enum class TokenType {
    // DML Keywords
    SELECT, FROM, WHERE, AND, OR, NOT, 
    JOIN, INNER, LEFT, RIGHT, FULL, ON,
    GROUP, BY, ORDER, ASC, DESC, HAVING,
    SUM, COUNT, AVG, MIN, MAX, LIMIT, OFFSET,
    AS, DISTINCT, IN, LIKE, NULL_KW,
    
    // DDL Keywords
    CREATE, TABLE, INSERT, INTO, VALUES,
    UPDATE, SET, DELETE, DROP, INDEX,
    IF, EXISTS,
    
    // Data Types
    INT, BIGINT, FLOAT_TYPE, DOUBLE, VARCHAR, BOOL_TYPE,
    
    // Operators
    EQUAL, NOT_EQUAL, LESS, GREATER, LESS_EQUAL, GREATER_EQUAL,
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    
    // Delimiters
    LPAREN, RPAREN, COMMA, DOT, STAR, SEMICOLON,
    
    // Literals
    INTEGER, FLOAT, STRING, IDENTIFIER,
    
    // Special
    END_OF_INPUT, ERROR
};

/**
 * @brief Token representation
 */
struct Token {
    TokenType type;
    std::string value;
    uint32_t line;
    uint32_t column;
    
    Token() : type(TokenType::ERROR), line(0), column(0) {}
    Token(TokenType t, const std::string& v, uint32_t l = 0, uint32_t c = 0)
        : type(t), value(v), line(l), column(c) {}
};

/**
 * @brief SQL lexical analyzer (tokenizer)
 */
class SqlLexer {
public:
    /**
     * @brief Tokenize SQL query string
     * @param query SQL query text
     * @return Vector of tokens
     */
    std::vector<Token> tokenize(const std::string& query);

private:
    std::string input_;
    size_t position_;
    uint32_t line_;
    uint32_t column_;
    
    // Lexer state machine
    char current_char();
    char peek_char(size_t offset = 1);
    void advance();
    void skip_whitespace();
    void skip_comment();
    
    // Token construction
    Token read_string();
    Token read_number();
    Token read_identifier();
    Token read_operator();
    
    // Keyword recognition
    static TokenType get_keyword_type(const std::string& word);
};

}  // namespace query
}  // namespace lyradb
