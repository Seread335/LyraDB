#pragma once

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace lyradb {
namespace optimization {

/**
 * @brief Query rewrite rules engine for Phase 4.4 Task 3
 * 
 * Transforms predicates to enable better index utilization:
 * 
 * Equivalence Rules:
 *   NOT(a < b)      → a >= b
 *   NOT(a <= b)     → a > b
 *   NOT(a = b)      → a != b
 *   NOT(a IN (x,y)) → a NOT IN (x,y)
 *   NOT(NOT(a))     → a
 *   
 * Normalization:
 *   Convert to DNF (Disjunctive Normal Form)
 *   Convert to CNF (Conjunctive Normal Form)
 *   Flatten nested AND/OR expressions
 *   
 * Optimization:
 *   Filter pushdown (move filters closer to scans)
 *   Predicate pushdown (execute early to reduce rows)
 *   Dead code elimination (remove redundant predicates)
 *   Constant folding (evaluate constants at compile time)
 */
class QueryRewriter {
public:
    /**
     * @brief Expression node for query tree representation
     */
    enum class ExprType {
        PREDICATE,      // Leaf: column OP value
        AND,            // Binary: expr AND expr
        OR,             // Binary: expr OR expr
        NOT             // Unary: NOT expr
    };
    
    /**
     * @brief Comparison operators
     */
    enum class CompOp {
        EQ,      // =
        NE,      // !=
        LT,      // <
        LE,      // <=
        GT,      // >
        GE,      // >=
        IN,      // IN (list)
        NIN      // NOT IN (list)
    };
    
    /**
     * @brief Single comparison predicate
     */
    struct Predicate {
        std::string column;
        CompOp op;
        std::string value;
        
        std::string to_string() const;
        static Predicate from_string(const std::string& str);
    };
    
    /**
     * @brief Expression tree node
     */
    struct Expr {
        ExprType type;
        
        // For PREDICATE nodes
        Predicate pred;
        
        // For AND/OR/NOT nodes
        std::shared_ptr<Expr> left;
        std::shared_ptr<Expr> right;  // right is null for NOT
        
        Expr();
        explicit Expr(const Predicate& p);
        explicit Expr(ExprType t);
        
        std::string to_string() const;
        std::shared_ptr<Expr> clone() const;
    };
    
    QueryRewriter();
    ~QueryRewriter();
    
    /**
     * @brief Apply equivalence transformations to expression
     * 
     * Examples:
     *   NOT(a < b) → a >= b
     *   NOT(NOT(a)) → a
     *   a AND a → a (idempotence)
     */
    std::shared_ptr<Expr> apply_equivalences(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Negate a predicate (implements De Morgan's laws)
     * 
     * NOT(a = b) → a != b
     * NOT(a < b) → a >= b
     * NOT(a AND b) → NOT(a) OR NOT(b)
     * NOT(a OR b) → NOT(a) AND NOT(b)
     */
    std::shared_ptr<Expr> negate_expr(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Negate a single predicate
     */
    static Predicate negate_predicate(const Predicate& pred);
    
    /**
     * @brief Convert expression to Disjunctive Normal Form (DNF)
     * 
     * OR of AND clauses: (A AND B) OR (C AND D)
     * Useful for: Determining which rows satisfy at least one condition
     */
    std::shared_ptr<Expr> to_dnf(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Convert expression to Conjunctive Normal Form (CNF)
     * 
     * AND of OR clauses: (A OR B) AND (C OR D)
     * Useful for: Pruning rows that don't satisfy all conditions
     */
    std::shared_ptr<Expr> to_cnf(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Push down filters in expression tree
     * 
     * Strategy: Move AND predicates down to leaf nodes
     * Benefit: Evaluate selective predicates first
     * 
     * Example: (A OR B) AND C → (A AND C) OR (B AND C)
     */
    std::shared_ptr<Expr> pushdown_filters(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Reorder predicates by estimated selectivity
     * 
     * Strategy: Most selective (= and <) before less selective (OR, !=)
     * Benefit: Reduce working set early
     */
    std::shared_ptr<Expr> reorder_by_selectivity(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Flatten nested AND expressions
     * 
     * Example: ((A AND B) AND C) → (A AND B AND C)
     * Result: Vector of flat predicates
     */
    std::vector<Predicate> flatten_and_clauses(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Flatten nested OR expressions
     * 
     * Example: ((A OR B) OR C) → (A OR B OR C)
     * Result: Vector of flat predicates
     */
    std::vector<Predicate> flatten_or_clauses(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Remove redundant predicates
     * 
     * Example: a > 10 AND a > 5 → a > 10
     * Identifies: Dominated, contradictory, or tautologous conditions
     */
    std::shared_ptr<Expr> eliminate_redundant(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Check if two predicates are contradictory
     * 
     * Example: a = 5 AND a = 10 (impossible)
     * Example: a > 100 AND a < 50 (impossible)
     */
    static bool are_contradictory(const Predicate& p1, const Predicate& p2);
    
    /**
     * @brief Check if predicate p2 is dominated by p1
     * 
     * Example: a > 10 dominates a > 5 (more restrictive)
     */
    static bool is_dominated(const Predicate& dominant, const Predicate& redundant);

private:
    /**
     * @brief Convert expression to DNF recursively
     */
    std::shared_ptr<Expr> to_dnf_recursive(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Convert expression to CNF recursively
     */
    std::shared_ptr<Expr> to_cnf_recursive(const std::shared_ptr<Expr>& expr);
    
    /**
     * @brief Distribute AND over OR: (A AND (B OR C)) → (A AND B) OR (A AND C)
     */
    std::shared_ptr<Expr> distribute_and_over_or(
        const std::shared_ptr<Expr>& and_expr,
        const std::shared_ptr<Expr>& or_expr);
    
    /**
     * @brief Distribute OR over AND: (A OR (B AND C)) → (A OR B) AND (A OR C)
     */
    std::shared_ptr<Expr> distribute_or_over_and(
        const std::shared_ptr<Expr>& or_expr,
        const std::shared_ptr<Expr>& and_expr);
    
    /**
     * @brief Get estimated selectivity of predicate
     */
    static double get_selectivity(const Predicate& pred);
};

} // namespace optimization
} // namespace lyradb
