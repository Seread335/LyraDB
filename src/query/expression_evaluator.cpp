#include "lyradb/expression_evaluator.h"
#include "lyradb/sql_parser.h"
#include <cmath>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <limits>

namespace lyradb {

ExpressionValue ExpressionEvaluator::evaluate(const query::Expression* expr, const RowData& row) {
    if (!expr) {
        return nullptr;
    }
    
    // Try to cast to specific expression types
    if (auto binary = dynamic_cast<const query::BinaryExpr*>(expr)) {
        return eval_binary(binary, row);
    } else if (auto unary = dynamic_cast<const query::UnaryExpr*>(expr)) {
        return eval_unary(unary, row);
    } else if (auto col_ref = dynamic_cast<const query::ColumnRefExpr*>(expr)) {
        return eval_column_ref(col_ref, row);
    } else if (auto literal = dynamic_cast<const query::LiteralExpr*>(expr)) {
        return eval_literal(literal);
    } else if (auto func = dynamic_cast<const query::FunctionExpr*>(expr)) {
        return eval_function(func, row);
    } else if (auto agg = dynamic_cast<const query::AggregateExpr*>(expr)) {
        // For single-row context, return 0 for aggregates
        // (Full aggregate evaluation requires batch context)
        return 0LL;
    }
    
    last_error_ = "Unknown expression type";
    return nullptr;
}

std::vector<ExpressionValue> ExpressionEvaluator::evaluate_batch(
    const query::Expression* expr,
    const std::vector<RowData>& rows) {
    
    std::vector<ExpressionValue> results;
    results.reserve(rows.size());
    
    for (const auto& row : rows) {
        results.push_back(evaluate(expr, row));
    }
    
    return results;
}

void ExpressionEvaluator::set_context_row(const RowData& row) {
    context_row_ = row;
}

std::string ExpressionEvaluator::get_last_error() const {
    return last_error_;
}

ExpressionValue ExpressionEvaluator::eval_binary(const query::BinaryExpr* expr, const RowData& row) {
    if (!expr || !expr->left || !expr->right) {
        return nullptr;
    }
    
    auto left = evaluate(expr->left.get(), row);
    auto right = evaluate(expr->right.get(), row);
    
    // Handle NULL values
    if (is_null(left) || is_null(right)) {
        // Most operations with NULL result in NULL, except AND/OR
        if (expr->op == query::BinaryOp::AND || expr->op == query::BinaryOp::OR) {
            return logical_and(left, right);  // Handles NULL propagation
        }
        return nullptr;
    }
    
    switch (expr->op) {
        case query::BinaryOp::ADD:
            return add(left, right);
        case query::BinaryOp::SUBTRACT:
            return subtract(left, right);
        case query::BinaryOp::MULTIPLY:
            return multiply(left, right);
        case query::BinaryOp::DIVIDE:
            return divide(left, right);
        case query::BinaryOp::MODULO:
            return modulo(left, right);
        case query::BinaryOp::EQUAL:
            return compare_equal(left, right);
        case query::BinaryOp::NOT_EQUAL:
            return !to_bool(compare_equal(left, right));
        case query::BinaryOp::LESS:
            return compare_less(left, right);
        case query::BinaryOp::GREATER:
            return compare_greater(left, right);
        case query::BinaryOp::LESS_EQUAL:
            return to_bool(compare_less(left, right)) || to_bool(compare_equal(left, right));
        case query::BinaryOp::GREATER_EQUAL:
            return to_bool(compare_greater(left, right)) || to_bool(compare_equal(left, right));
        case query::BinaryOp::AND:
            return logical_and(left, right);
        case query::BinaryOp::OR:
            return logical_or(left, right);
        case query::BinaryOp::LIKE:
            return string_like(left, right);
        case query::BinaryOp::IN:
            // IN operator not fully implemented
            return false;
    }
    
    return nullptr;
}

ExpressionValue ExpressionEvaluator::eval_unary(const query::UnaryExpr* expr, const RowData& row) {
    if (!expr || !expr->operand) {
        return nullptr;
    }
    
    auto operand = evaluate(expr->operand.get(), row);
    
    switch (expr->op) {
        case query::UnaryOp::NOT:
            return logical_not(operand);
        case query::UnaryOp::NEGATE:
            return ExpressionValue(-to_double(operand));
    }
    
    return nullptr;
}

ExpressionValue ExpressionEvaluator::eval_column_ref(const query::ColumnRefExpr* expr, const RowData& row) {
    if (!expr) {
        return nullptr;
    }
    
    auto it = row.find(expr->column_name);
    if (it != row.end()) {
        return it->second;
    }
    
    last_error_ = "Column not found: " + expr->column_name;
    return nullptr;
}

ExpressionValue ExpressionEvaluator::eval_literal(const query::LiteralExpr* expr) {
    if (!expr) {
        return nullptr;
    }
    
    const auto& token = expr->value;
    
    // Parse token based on its type
    if (token.type == query::TokenType::INTEGER) {
        try {
            return std::stoll(token.value);
        } catch (...) {
            last_error_ = "Invalid integer: " + token.value;
            return nullptr;
        }
    } else if (token.type == query::TokenType::FLOAT) {
        try {
            return std::stod(token.value);
        } catch (...) {
            last_error_ = "Invalid float: " + token.value;
            return nullptr;
        }
    } else if (token.type == query::TokenType::STRING) {
        // Remove quotes from string literal
        std::string str = token.value;
        if ((str.front() == '\'' && str.back() == '\'') ||
            (str.front() == '"' && str.back() == '"')) {
            str = str.substr(1, str.length() - 2);
        }
        return str;
    } else if (token.type == query::TokenType::SELECT) {
        // Could be a keyword used as boolean true (not standard SQL)
        return true;
    } else if (token.type == query::TokenType::NULL_KW) {
        return nullptr;
    }
    
    return nullptr;
}

ExpressionValue ExpressionEvaluator::eval_function(const query::FunctionExpr* expr, const RowData& row) {
    if (!expr) {
        return nullptr;
    }
    
    const auto& func_name = expr->function_name;
    std::vector<ExpressionValue> args;
    
    // Evaluate function arguments
    for (const auto& arg_expr : expr->arguments) {
        args.push_back(evaluate(arg_expr.get(), row));
    }
    
    // String functions
    if (func_name == "UPPER" && args.size() >= 1) {
        return func_upper(args[0]);
    } else if (func_name == "LOWER" && args.size() >= 1) {
        return func_lower(args[0]);
    } else if (func_name == "LENGTH" && args.size() >= 1) {
        return func_length(args[0]);
    } else if (func_name == "SUBSTR" && args.size() >= 2) {
        return func_substr(args);
    } else if (func_name == "ROUND" && args.size() >= 1) {
        return func_round(args);
    } else if (func_name == "ABS" && args.size() >= 1) {
        return func_abs(args[0]);
    } else if (func_name == "COALESCE" && args.size() >= 1) {
        return func_coalesce(args);
    }
    
    last_error_ = "Unknown function: " + func_name;
    return nullptr;
}

ExpressionValue ExpressionEvaluator::eval_aggregate(const query::AggregateExpr* expr,
                                                     const std::vector<RowData>& rows) {
    if (!expr || rows.empty()) {
        return nullptr;
    }
    
    // Evaluate aggregate function
    switch (expr->aggregate_func) {
        case query::AggregateFunc::COUNT: {
            return static_cast<int64_t>(rows.size());
        }
        case query::AggregateFunc::SUM: {
            double sum = 0.0;
            for (const auto& row : rows) {
                auto val = evaluate(expr->argument.get(), row);
                sum += to_double(val);
            }
            return sum;
        }
        case query::AggregateFunc::AVG: {
            double sum = 0.0;
            for (const auto& row : rows) {
                auto val = evaluate(expr->argument.get(), row);
                sum += to_double(val);
            }
            return rows.empty() ? 0.0 : (sum / rows.size());
        }
        case query::AggregateFunc::MIN: {
            double min_val = std::numeric_limits<double>::max();
            for (const auto& row : rows) {
                auto val = evaluate(expr->argument.get(), row);
                min_val = std::min(min_val, to_double(val));
            }
            return min_val == std::numeric_limits<double>::max() ? 0.0 : min_val;
        }
        case query::AggregateFunc::MAX: {
            double max_val = std::numeric_limits<double>::lowest();
            for (const auto& row : rows) {
                auto val = evaluate(expr->argument.get(), row);
                max_val = std::max(max_val, to_double(val));
            }
            return max_val == std::numeric_limits<double>::lowest() ? 0.0 : max_val;
        }
    }
    
    return nullptr;
}

// Type coercion helpers
double ExpressionEvaluator::to_double(const ExpressionValue& val) const {
    if (std::holds_alternative<std::nullptr_t>(val)) return 0.0;
    if (std::holds_alternative<int64_t>(val)) return static_cast<double>(std::get<int64_t>(val));
    if (std::holds_alternative<double>(val)) return std::get<double>(val);
    if (std::holds_alternative<std::string>(val)) {
        try {
            return std::stod(std::get<std::string>(val));
        } catch (...) {
            return 0.0;
        }
    }
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? 1.0 : 0.0;
    return 0.0;
}

int64_t ExpressionEvaluator::to_int64(const ExpressionValue& val) const {
    if (std::holds_alternative<std::nullptr_t>(val)) return 0;
    if (std::holds_alternative<int64_t>(val)) return std::get<int64_t>(val);
    if (std::holds_alternative<double>(val)) return static_cast<int64_t>(std::get<double>(val));
    if (std::holds_alternative<std::string>(val)) {
        try {
            return std::stoll(std::get<std::string>(val));
        } catch (...) {
            return 0;
        }
    }
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? 1 : 0;
    return 0;
}

std::string ExpressionEvaluator::to_string(const ExpressionValue& val) const {
    if (std::holds_alternative<std::nullptr_t>(val)) return "NULL";
    if (std::holds_alternative<int64_t>(val)) return std::to_string(std::get<int64_t>(val));
    if (std::holds_alternative<double>(val)) {
        std::ostringstream oss;
        oss << std::get<double>(val);
        return oss.str();
    }
    if (std::holds_alternative<std::string>(val)) return std::get<std::string>(val);
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val) ? "true" : "false";
    return "NULL";
}

bool ExpressionEvaluator::to_bool(const ExpressionValue& val) const {
    if (std::holds_alternative<std::nullptr_t>(val)) return false;
    if (std::holds_alternative<int64_t>(val)) return std::get<int64_t>(val) != 0;
    if (std::holds_alternative<double>(val)) return std::get<double>(val) != 0.0;
    if (std::holds_alternative<std::string>(val)) return !std::get<std::string>(val).empty();
    if (std::holds_alternative<bool>(val)) return std::get<bool>(val);
    return false;
}

bool ExpressionEvaluator::is_null(const ExpressionValue& val) const {
    return std::holds_alternative<std::nullptr_t>(val);
}

// Arithmetic operations
ExpressionValue ExpressionEvaluator::add(const ExpressionValue& left, const ExpressionValue& right) const {
    // Handle string concatenation
    if (std::holds_alternative<std::string>(left) || std::holds_alternative<std::string>(right)) {
        return string_concat(left, right);
    }
    return ExpressionValue(to_double(left) + to_double(right));
}

ExpressionValue ExpressionEvaluator::subtract(const ExpressionValue& left, const ExpressionValue& right) const {
    return ExpressionValue(to_double(left) - to_double(right));
}

ExpressionValue ExpressionEvaluator::multiply(const ExpressionValue& left, const ExpressionValue& right) const {
    return ExpressionValue(to_double(left) * to_double(right));
}

ExpressionValue ExpressionEvaluator::divide(const ExpressionValue& left, const ExpressionValue& right) const {
    double r = to_double(right);
    if (std::abs(r) < 1e-9) {
        last_error_ = "Division by zero";
        return nullptr;
    }
    return ExpressionValue(to_double(left) / r);
}

ExpressionValue ExpressionEvaluator::modulo(const ExpressionValue& left, const ExpressionValue& right) const {
    int64_t l = to_int64(left);
    int64_t r = to_int64(right);
    if (r == 0) {
        last_error_ = "Modulo by zero";
        return nullptr;
    }
    return ExpressionValue(l % r);
}

// Comparison operations
ExpressionValue ExpressionEvaluator::compare_equal(const ExpressionValue& left, const ExpressionValue& right) const {
    // Compare as doubles for numeric comparison
    if ((std::holds_alternative<int64_t>(left) || std::holds_alternative<double>(left)) &&
        (std::holds_alternative<int64_t>(right) || std::holds_alternative<double>(right))) {
        return ExpressionValue(std::abs(to_double(left) - to_double(right)) < 1e-9);
    }
    
    // String comparison
    return ExpressionValue(to_string(left) == to_string(right));
}

ExpressionValue ExpressionEvaluator::compare_less(const ExpressionValue& left, const ExpressionValue& right) const {
    return ExpressionValue(to_double(left) < to_double(right));
}

ExpressionValue ExpressionEvaluator::compare_greater(const ExpressionValue& left, const ExpressionValue& right) const {
    return ExpressionValue(to_double(left) > to_double(right));
}

// Logical operations
ExpressionValue ExpressionEvaluator::logical_and(const ExpressionValue& left, const ExpressionValue& right) const {
    // NULL handling: NULL AND false = false, NULL AND true = NULL
    if (is_null(left) || is_null(right)) {
        if (!to_bool(left) || !to_bool(right)) {
            return false;
        }
        return nullptr;
    }
    return ExpressionValue(to_bool(left) && to_bool(right));
}

ExpressionValue ExpressionEvaluator::logical_or(const ExpressionValue& left, const ExpressionValue& right) const {
    // NULL handling: NULL OR true = true, NULL OR false = NULL
    if (to_bool(left) || to_bool(right)) {
        return true;
    }
    if (is_null(left) || is_null(right)) {
        return nullptr;
    }
    return false;
}

ExpressionValue ExpressionEvaluator::logical_not(const ExpressionValue& val) const {
    if (is_null(val)) {
        return nullptr;  // NOT NULL = NULL
    }
    return ExpressionValue(!to_bool(val));
}

// String operations
ExpressionValue ExpressionEvaluator::string_concat(const ExpressionValue& left, const ExpressionValue& right) const {
    return ExpressionValue(to_string(left) + to_string(right));
}

ExpressionValue ExpressionEvaluator::string_like(const ExpressionValue& str, const ExpressionValue& pattern) const {
    std::string s = to_string(str);
    std::string p = to_string(pattern);
    
    // Simple LIKE pattern matching (% = any chars, _ = single char)
    // For simplicity, just check if pattern is substring
    if (p.find('%') == std::string::npos && p.find('_') == std::string::npos) {
        return ExpressionValue(s.find(p) != std::string::npos);
    }
    
    // More complex pattern matching would go here
    return ExpressionValue(false);
}

// Function implementations
ExpressionValue ExpressionEvaluator::func_upper(const ExpressionValue& val) const {
    std::string s = to_string(val);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::toupper(c);
    });
    return ExpressionValue(s);
}

ExpressionValue ExpressionEvaluator::func_lower(const ExpressionValue& val) const {
    std::string s = to_string(val);
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) {
        return std::tolower(c);
    });
    return ExpressionValue(s);
}

ExpressionValue ExpressionEvaluator::func_length(const ExpressionValue& val) const {
    return ExpressionValue(static_cast<int64_t>(to_string(val).length()));
}

ExpressionValue ExpressionEvaluator::func_substr(const std::vector<ExpressionValue>& args) const {
    if (args.empty()) return nullptr;
    
    std::string s = to_string(args[0]);
    int64_t start = args.size() > 1 ? to_int64(args[1]) : 1;
    int64_t length = args.size() > 2 ? to_int64(args[2]) : s.length();
    
    // SQL SUBSTR is 1-indexed
    start = std::max(1LL, start) - 1;  // Convert to 0-indexed
    if (start >= static_cast<int64_t>(s.length())) {
        return ExpressionValue(std::string(""));
    }
    
    return ExpressionValue(s.substr(start, length));
}

ExpressionValue ExpressionEvaluator::func_round(const std::vector<ExpressionValue>& args) const {
    if (args.empty()) return nullptr;
    
    double val = to_double(args[0]);
    int64_t decimals = args.size() > 1 ? to_int64(args[1]) : 0;
    
    double multiplier = std::pow(10.0, decimals);
    return ExpressionValue(std::round(val * multiplier) / multiplier);
}

ExpressionValue ExpressionEvaluator::func_abs(const ExpressionValue& val) const {
    return ExpressionValue(std::abs(to_double(val)));
}

ExpressionValue ExpressionEvaluator::func_coalesce(const std::vector<ExpressionValue>& args) const {
    for (const auto& val : args) {
        if (!is_null(val)) {
            return val;
        }
    }
    return nullptr;
}

} // namespace lyradb
