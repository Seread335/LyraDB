#pragma once

#include <memory>
#include <string>
#include <vector>
#include <variant>
#include <unordered_map>
#include <cstdint>

namespace lyradb {

// Forward declarations
namespace query {
    class Expression;
    class BinaryExpr;
    class UnaryExpr;
    class ColumnRefExpr;
    class LiteralExpr;
    class FunctionExpr;
    class AggregateExpr;
}

/**
 * @brief Value type for expression evaluation
 * Supports: integers, doubles, strings, booleans, NULL
 */
using ExpressionValue = std::variant<
    std::nullptr_t,
    int64_t,
    double,
    std::string,
    bool
>;

/**
 * @brief Row data container
 * Maps column names to their values
 */
using RowData = std::unordered_map<std::string, ExpressionValue>;

/**
 * @brief Expression Evaluator
 * 
 * Evaluates SQL expressions (e.g., WHERE clauses, computed columns)
 * Supports:
 * - Scalar evaluation (single row)
 * - Batch evaluation (vectorized processing)
 * - Type coercion and NULL handling
 * - Function calls and aggregate functions
 */
class ExpressionEvaluator {
public:
    ExpressionEvaluator() = default;
    ~ExpressionEvaluator() = default;
    
    /**
     * @brief Evaluate an expression for a single row
     * @param expr The expression to evaluate
     * @param row Row data mapping column names to values
     * @return Evaluation result
     */
    ExpressionValue evaluate(const query::Expression* expr, const RowData& row);
    
    /**
     * @brief Evaluate an expression for multiple rows (vectorized)
     * @param expr The expression to evaluate
     * @param rows Vector of row data
     * @return Vector of evaluation results
     */
    std::vector<ExpressionValue> evaluate_batch(
        const query::Expression* expr,
        const std::vector<RowData>& rows);
    
    /**
     * @brief Set context row for evaluation
     * @param row Row data to use as evaluation context
     */
    void set_context_row(const RowData& row);
    
    /**
     * @brief Get last error message
     * @return Error message string
     */
    std::string get_last_error() const;

private:
    RowData context_row_;
    mutable std::string last_error_;
    
    // Recursive evaluation methods
    ExpressionValue eval_binary(const query::BinaryExpr* expr, const RowData& row);
    ExpressionValue eval_unary(const query::UnaryExpr* expr, const RowData& row);
    ExpressionValue eval_column_ref(const query::ColumnRefExpr* expr, const RowData& row);
    ExpressionValue eval_literal(const query::LiteralExpr* expr);
    ExpressionValue eval_function(const query::FunctionExpr* expr, const RowData& row);
    ExpressionValue eval_aggregate(const query::AggregateExpr* expr, 
                                   const std::vector<RowData>& rows);
    
    // Type coercion helpers
    double to_double(const ExpressionValue& val) const;
    int64_t to_int64(const ExpressionValue& val) const;
    std::string to_string(const ExpressionValue& val) const;
    bool to_bool(const ExpressionValue& val) const;
    bool is_null(const ExpressionValue& val) const;
    
    // Arithmetic operations
    ExpressionValue add(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue subtract(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue multiply(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue divide(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue modulo(const ExpressionValue& left, const ExpressionValue& right) const;
    
    // Comparison operations
    ExpressionValue compare_equal(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue compare_less(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue compare_greater(const ExpressionValue& left, const ExpressionValue& right) const;
    
    // Logical operations
    ExpressionValue logical_and(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue logical_or(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue logical_not(const ExpressionValue& val) const;
    
    // String operations
    ExpressionValue string_concat(const ExpressionValue& left, const ExpressionValue& right) const;
    ExpressionValue string_like(const ExpressionValue& str, const ExpressionValue& pattern) const;
    
    // Function implementations
    ExpressionValue func_upper(const ExpressionValue& val) const;
    ExpressionValue func_lower(const ExpressionValue& val) const;
    ExpressionValue func_length(const ExpressionValue& val) const;
    ExpressionValue func_substr(const std::vector<ExpressionValue>& args) const;
    ExpressionValue func_round(const std::vector<ExpressionValue>& args) const;
    ExpressionValue func_abs(const ExpressionValue& val) const;
    ExpressionValue func_coalesce(const std::vector<ExpressionValue>& args) const;
};

} // namespace lyradb
