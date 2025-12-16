#include "lyradb/sql_lexer.h"
#include <cctype>
#include <map>

namespace lyradb {
namespace query {

std::vector<Token> SqlLexer::tokenize(const std::string& query) {
    input_ = query;
    position_ = 0;
    line_ = 1;
    column_ = 1;
    
    std::vector<Token> tokens;
    
    while (position_ < input_.length()) {
        skip_whitespace();
        
        if (position_ >= input_.length()) break;
        
        char ch = current_char();
        
        // String literals
        if (ch == '\'' || ch == '"') {
            tokens.push_back(read_string());
        }
        // Numbers
        else if (std::isdigit(ch)) {
            tokens.push_back(read_number());
        }
        // Identifiers and keywords
        else if (std::isalpha(ch) || ch == '_') {
            tokens.push_back(read_identifier());
        }
        // Operators and delimiters
        else {
            Token token = read_operator();
            if (token.type != TokenType::ERROR) {
                tokens.push_back(token);
            }
        }
    }
    
    tokens.push_back(Token(TokenType::END_OF_INPUT, "", line_, column_));
    return tokens;
}

char SqlLexer::current_char() {
    if (position_ >= input_.length()) return '\0';
    return input_[position_];
}

char SqlLexer::peek_char(size_t offset) {
    size_t pos = position_ + offset;
    if (pos >= input_.length()) return '\0';
    return input_[pos];
}

void SqlLexer::advance() {
    if (position_ < input_.length()) {
        if (input_[position_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        position_++;
    }
}

void SqlLexer::skip_whitespace() {
    while (position_ < input_.length() && std::isspace(current_char())) {
        advance();
    }
    
    // Skip comments
    if (current_char() == '-' && peek_char() == '-') {
        skip_comment();
        skip_whitespace();
    }
}

void SqlLexer::skip_comment() {
    while (position_ < input_.length() && current_char() != '\n') {
        advance();
    }
    if (current_char() == '\n') {
        advance();
    }
}

Token SqlLexer::read_string() {
    char quote_char = current_char();
    uint32_t start_line = line_;
    uint32_t start_col = column_;
    advance();  // Skip opening quote
    
    std::string value;
    while (position_ < input_.length() && current_char() != quote_char) {
        if (current_char() == '\\' && peek_char() == quote_char) {
            advance();  // Skip backslash
            value += current_char();
            advance();
        } else {
            value += current_char();
            advance();
        }
    }
    
    if (current_char() == quote_char) {
        advance();  // Skip closing quote
    }
    
    return Token(TokenType::STRING, value, start_line, start_col);
}

Token SqlLexer::read_number() {
    uint32_t start_line = line_;
    uint32_t start_col = column_;
    std::string value;
    bool is_float = false;
    
    while (position_ < input_.length() && 
           (std::isdigit(current_char()) || current_char() == '.')) {
        if (current_char() == '.') {
            if (is_float) break;  // Second dot
            is_float = true;
        }
        value += current_char();
        advance();
    }
    
    return Token(is_float ? TokenType::FLOAT : TokenType::INTEGER, 
                 value, start_line, start_col);
}

Token SqlLexer::read_identifier() {
    uint32_t start_line = line_;
    uint32_t start_col = column_;
    std::string value;
    
    while (position_ < input_.length() && 
           (std::isalnum(current_char()) || current_char() == '_')) {
        value += current_char();
        advance();
    }
    
    // Convert to uppercase for keyword matching
    std::string upper_value = value;
    for (auto& c : upper_value) {
        c = std::toupper(c);
    }
    
    TokenType type = get_keyword_type(upper_value);
    return Token(type, value, start_line, start_col);
}

Token SqlLexer::read_operator() {
    uint32_t start_line = line_;
    uint32_t start_col = column_;
    char ch = current_char();
    
    if (ch == '(') {
        advance();
        return Token(TokenType::LPAREN, "(", start_line, start_col);
    } else if (ch == ')') {
        advance();
        return Token(TokenType::RPAREN, ")", start_line, start_col);
    } else if (ch == ',') {
        advance();
        return Token(TokenType::COMMA, ",", start_line, start_col);
    } else if (ch == '.') {
        advance();
        return Token(TokenType::DOT, ".", start_line, start_col);
    } else if (ch == '*') {
        advance();
        return Token(TokenType::STAR, "*", start_line, start_col);
    } else if (ch == ';') {
        advance();
        return Token(TokenType::SEMICOLON, ";", start_line, start_col);
    } else if (ch == '+') {
        advance();
        return Token(TokenType::PLUS, "+", start_line, start_col);
    } else if (ch == '-') {
        advance();
        return Token(TokenType::MINUS, "-", start_line, start_col);
    } else if (ch == '/') {
        advance();
        return Token(TokenType::DIVIDE, "/", start_line, start_col);
    } else if (ch == '%') {
        advance();
        return Token(TokenType::MODULO, "%", start_line, start_col);
    } else if (ch == '=') {
        advance();
        return Token(TokenType::EQUAL, "=", start_line, start_col);
    } else if (ch == '<') {
        advance();
        if (current_char() == '=') {
            advance();
            return Token(TokenType::LESS_EQUAL, "<=", start_line, start_col);
        } else if (current_char() == '>') {
            advance();
            return Token(TokenType::NOT_EQUAL, "<>", start_line, start_col);
        }
        return Token(TokenType::LESS, "<", start_line, start_col);
    } else if (ch == '>') {
        advance();
        if (current_char() == '=') {
            advance();
            return Token(TokenType::GREATER_EQUAL, ">=", start_line, start_col);
        }
        return Token(TokenType::GREATER, ">", start_line, start_col);
    } else if (ch == '!') {
        advance();
        if (current_char() == '=') {
            advance();
            return Token(TokenType::NOT_EQUAL, "!=", start_line, start_col);
        }
        return Token(TokenType::ERROR, "!", start_line, start_col);
    }
    
    advance();
    return Token(TokenType::ERROR, std::string(1, ch), start_line, start_col);
}

TokenType SqlLexer::get_keyword_type(const std::string& word) {
    static const std::map<std::string, TokenType> keywords = {
        // DML Keywords
        {"SELECT", TokenType::SELECT},
        {"FROM", TokenType::FROM},
        {"WHERE", TokenType::WHERE},
        {"AND", TokenType::AND},
        {"OR", TokenType::OR},
        {"NOT", TokenType::NOT},
        {"JOIN", TokenType::JOIN},
        {"INNER", TokenType::INNER},
        {"LEFT", TokenType::LEFT},
        {"RIGHT", TokenType::RIGHT},
        {"FULL", TokenType::FULL},
        {"ON", TokenType::ON},
        {"GROUP", TokenType::GROUP},
        {"BY", TokenType::BY},
        {"ORDER", TokenType::ORDER},
        {"ASC", TokenType::ASC},
        {"DESC", TokenType::DESC},
        {"HAVING", TokenType::HAVING},
        {"SUM", TokenType::SUM},
        {"COUNT", TokenType::COUNT},
        {"AVG", TokenType::AVG},
        {"MIN", TokenType::MIN},
        {"MAX", TokenType::MAX},
        {"LIMIT", TokenType::LIMIT},
        {"OFFSET", TokenType::OFFSET},
        {"AS", TokenType::AS},
        {"DISTINCT", TokenType::DISTINCT},
        {"IN", TokenType::IN},
        {"LIKE", TokenType::LIKE},
        {"NULL", TokenType::NULL_KW},
        
        // DDL Keywords
        {"CREATE", TokenType::CREATE},
        {"TABLE", TokenType::TABLE},
        {"INSERT", TokenType::INSERT},
        {"INTO", TokenType::INTO},
        {"VALUES", TokenType::VALUES},
        {"UPDATE", TokenType::UPDATE},
        {"SET", TokenType::SET},
        {"DELETE", TokenType::DELETE},
        {"DROP", TokenType::DROP},
        {"INDEX", TokenType::INDEX},
        {"IF", TokenType::IF},
        {"EXISTS", TokenType::EXISTS},
        
        // Data Types
        {"INT", TokenType::INT},
        {"BIGINT", TokenType::BIGINT},
        {"FLOAT", TokenType::FLOAT_TYPE},
        {"DOUBLE", TokenType::DOUBLE},
        {"VARCHAR", TokenType::VARCHAR},
        {"BOOL", TokenType::BOOL_TYPE},
    };
    
    auto it = keywords.find(word);
    if (it != keywords.end()) {
        return it->second;
    }
    
    return TokenType::IDENTIFIER;
}

}  // namespace query
}  // namespace lyradb
