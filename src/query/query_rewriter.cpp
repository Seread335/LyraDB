#include "lyradb/query_rewriter.h"
#include <sstream>
#include <algorithm>
#include <set>

namespace lyradb {
namespace optimization {

// ============= Predicate Implementation =============
std::string Predicate::to_string() const {
    std::stringstream ss;
    ss << column << " ";
    
    switch (op) {
        case CompOp::EQ:  ss << "="; break;
        case CompOp::NE:  ss << "!="; break;
        case CompOp::LT:  ss << "<"; break;
        case CompOp::LE:  ss << "<="; break;
        case CompOp::GT:  ss << ">"; break;
        case CompOp::GE:  ss << ">="; break;
        case CompOp::IN:  ss << "IN"; break;
        case CompOp::NIN: ss << "NOT IN"; break;
    }
    
    ss << " " << value;
    return ss.str();
}

Predicate Predicate::from_string(const std::string& str) {
    Predicate p;
    // Simple parser: "column OP value"
    std::istringstream iss(str);
    std::string op_str;
    
    iss >> p.column >> op_str >> p.value;
    
    if (op_str == "=") p.op = CompOp::EQ;
    else if (op_str == "!=") p.op = CompOp::NE;
    else if (op_str == "<") p.op = CompOp::LT;
    else if (op_str == "<=") p.op = CompOp::LE;
    else if (op_str == ">") p.op = CompOp::GT;
    else if (op_str == ">=") p.op = CompOp::GE;
    else if (op_str == "IN") p.op = CompOp::IN;
    else if (op_str == "NOT") p.op = CompOp::NIN;
    else p.op = CompOp::EQ;
    
    return p;
}

// ============= Expr Implementation =============
Expr::Expr() : type(ExprType::PREDICATE) {}

Expr::Expr(const Predicate& p) : type(ExprType::PREDICATE), pred(p) {}

Expr::Expr(ExprType t) : type(t), left(nullptr), right(nullptr) {}

std::string Expr::to_string() const {
    switch (type) {
        case ExprType::PREDICATE:
            return pred.to_string();
        case ExprType::AND:
            return "(" + left->to_string() + " AND " + right->to_string() + ")";
        case ExprType::OR:
            return "(" + left->to_string() + " OR " + right->to_string() + ")";
        case ExprType::NOT:
            return "NOT(" + left->to_string() + ")";
        default:
            return "UNKNOWN";
    }
}

std::shared_ptr<Expr> Expr::clone() const {
    auto result = std::make_shared<Expr>();
    result->type = type;
    result->pred = pred;
    
    if (left) {
        result->left = left->clone();
    }
    if (right) {
        result->right = right->clone();
    }
    
    return result;
}

// ============= QueryRewriter Implementation =============
QueryRewriter::QueryRewriter() {}

QueryRewriter::~QueryRewriter() {}

std::shared_ptr<Expr> QueryRewriter::apply_equivalences(const std::shared_ptr<Expr>& expr) {
    if (!expr) return expr;
    
    auto result = expr->clone();
    
    // Apply to children first (bottom-up)
    if (result->left) {
        result->left = apply_equivalences(result->left);
    }
    if (result->right) {
        result->right = apply_equivalences(result->right);
    }
    
    // Double negation elimination: NOT(NOT(a)) → a
    if (result->type == ExprType::NOT && result->left && result->left->type == ExprType::NOT) {
        return apply_equivalences(result->left->left);
    }
    
    // Idempotence: a AND a → a, a OR a → a
    if ((result->type == ExprType::AND || result->type == ExprType::OR) &&
        result->left && result->right &&
        result->left->to_string() == result->right->to_string()) {
        return result->left;
    }
    
    return result;
}

std::shared_ptr<Expr> QueryRewriter::negate_expr(const std::shared_ptr<Expr>& expr) {
    if (!expr) return expr;
    
    if (expr->type == ExprType::PREDICATE) {
        auto negated = std::make_shared<Expr>(negate_predicate(expr->pred));
        return negated;
    }
    
    if (expr->type == ExprType::NOT) {
        return expr->left;  // Double negation
    }
    
    if (expr->type == ExprType::AND) {
        // De Morgan: NOT(A AND B) = NOT(A) OR NOT(B)
        auto result = std::make_shared<Expr>(ExprType::OR);
        result->left = negate_expr(expr->left);
        result->right = negate_expr(expr->right);
        return result;
    }
    
    if (expr->type == ExprType::OR) {
        // De Morgan: NOT(A OR B) = NOT(A) AND NOT(B)
        auto result = std::make_shared<Expr>(ExprType::AND);
        result->left = negate_expr(expr->left);
        result->right = negate_expr(expr->right);
        return result;
    }
    
    return expr;
}

Predicate QueryRewriter::negate_predicate(const Predicate& pred) {
    Predicate result = pred;
    
    switch (pred.op) {
        case CompOp::EQ:  result.op = CompOp::NE; break;
        case CompOp::NE:  result.op = CompOp::EQ; break;
        case CompOp::LT:  result.op = CompOp::GE; break;
        case CompOp::LE:  result.op = CompOp::GT; break;
        case CompOp::GT:  result.op = CompOp::LE; break;
        case CompOp::GE:  result.op = CompOp::LT; break;
        case CompOp::IN:  result.op = CompOp::NIN; break;
        case CompOp::NIN: result.op = CompOp::IN; break;
    }
    
    return result;
}

std::shared_ptr<Expr> QueryRewriter::to_dnf(const std::shared_ptr<Expr>& expr) {
    if (!expr) return expr;
    return to_dnf_recursive(expr->clone());
}

std::shared_ptr<Expr> QueryRewriter::to_dnf_recursive(const std::shared_ptr<Expr>& expr) {
    if (!expr) return expr;
    
    if (expr->type == ExprType::PREDICATE) {
        return expr;
    }
    
    if (expr->type == ExprType::NOT) {
        if (expr->left) {
            expr->left = to_dnf_recursive(expr->left);
            expr = negate_expr(expr);
            return to_dnf_recursive(expr);
        }
    }
    
    // Recursively convert children
    if (expr->left) expr->left = to_dnf_recursive(expr->left);
    if (expr->right) expr->right = to_dnf_recursive(expr->right);
    
    // Apply distribution: AND over OR
    if (expr->type == ExprType::AND) {
        if (expr->left && expr->left->type == ExprType::OR) {
            return to_dnf_recursive(distribute_and_over_or(expr, expr->left));
        }
        if (expr->right && expr->right->type == ExprType::OR) {
            return to_dnf_recursive(distribute_and_over_or(expr, expr->right));
        }
    }
    
    return expr;
}

std::shared_ptr<Expr> QueryRewriter::to_cnf_recursive(const std::shared_ptr<Expr>& expr) {
    if (!expr) return expr;
    
    if (expr->type == ExprType::PREDICATE) {
        return expr;
    }
    
    if (expr->type == ExprType::NOT) {
        if (expr->left) {
            expr->left = to_cnf_recursive(expr->left);
            expr = negate_expr(expr);
            return to_cnf_recursive(expr);
        }
    }
    
    // Recursively convert children
    if (expr->left) expr->left = to_cnf_recursive(expr->left);
    if (expr->right) expr->right = to_cnf_recursive(expr->right);
    
    // Apply distribution: OR over AND
    if (expr->type == ExprType::OR) {
        if (expr->left && expr->left->type == ExprType::AND) {
            return to_cnf_recursive(distribute_or_over_and(expr, expr->left));
        }
        if (expr->right && expr->right->type == ExprType::AND) {
            return to_cnf_recursive(distribute_or_over_and(expr, expr->right));
        }
    }
    
    return expr;
}

std::shared_ptr<Expr> QueryRewriter::to_cnf(const std::shared_ptr<Expr>& expr) {
    if (!expr) return expr;
    return to_cnf_recursive(expr->clone());
}

std::shared_ptr<Expr> QueryRewriter::distribute_and_over_or(
    const std::shared_ptr<Expr>& and_expr,
    const std::shared_ptr<Expr>& or_expr) {
    
    // (A AND (B OR C)) → (A AND B) OR (A AND C)
    auto other = (and_expr->left == or_expr) ? and_expr->right : and_expr->left;
    
    auto left_and = std::make_shared<Expr>(ExprType::AND);
    left_and->left = other->clone();
    left_and->right = or_expr->left->clone();
    
    auto right_and = std::make_shared<Expr>(ExprType::AND);
    right_and->left = other->clone();
    right_and->right = or_expr->right->clone();
    
    auto result = std::make_shared<Expr>(ExprType::OR);
    result->left = left_and;
    result->right = right_and;
    
    return result;
}

std::shared_ptr<Expr> QueryRewriter::distribute_or_over_and(
    const std::shared_ptr<Expr>& or_expr,
    const std::shared_ptr<Expr>& and_expr) {
    
    // (A OR (B AND C)) → (A OR B) AND (A OR C)
    auto other = (or_expr->left == and_expr) ? or_expr->right : or_expr->left;
    
    auto left_or = std::make_shared<Expr>(ExprType::OR);
    left_or->left = other->clone();
    left_or->right = and_expr->left->clone();
    
    auto right_or = std::make_shared<Expr>(ExprType::OR);
    right_or->left = other->clone();
    right_or->right = and_expr->right->clone();
    
    auto result = std::make_shared<Expr>(ExprType::AND);
    result->left = left_or;
    result->right = right_or;
    
    return result;
}

std::shared_ptr<Expr> QueryRewriter::pushdown_filters(const std::shared_ptr<Expr>& expr) {
    if (!expr) return expr;
    
    auto result = expr->clone();
    
    // Recursively push down in children
    if (result->left) result->left = pushdown_filters(result->left);
    if (result->right) result->right = pushdown_filters(result->right);
    
    // Push AND down through OR: (A OR B) AND C → (A AND C) OR (B AND C)
    if (result->type == ExprType::AND && result->left && result->left->type == ExprType::OR) {
        auto left_and = std::make_shared<Expr>(ExprType::AND);
        left_and->left = result->left->left->clone();
        left_and->right = result->right->clone();
        
        auto right_and = std::make_shared<Expr>(ExprType::AND);
        right_and->left = result->left->right->clone();
        right_and->right = result->right->clone();
        
        auto or_expr = std::make_shared<Expr>(ExprType::OR);
        or_expr->left = left_and;
        or_expr->right = right_and;
        
        return pushdown_filters(or_expr);
    }
    
    if (result->type == ExprType::AND && result->right && result->right->type == ExprType::OR) {
        auto left_and = std::make_shared<Expr>(ExprType::AND);
        left_and->left = result->left->clone();
        left_and->right = result->right->left->clone();
        
        auto right_and = std::make_shared<Expr>(ExprType::AND);
        right_and->left = result->left->clone();
        right_and->right = result->right->right->clone();
        
        auto or_expr = std::make_shared<Expr>(ExprType::OR);
        or_expr->left = left_and;
        or_expr->right = right_and;
        
        return pushdown_filters(or_expr);
    }
    
    return result;
}

std::shared_ptr<Expr> QueryRewriter::reorder_by_selectivity(const std::shared_ptr<Expr>& expr) {
    if (!expr || expr->type != ExprType::AND) {
        return expr;
    }
    
    auto and_clauses = flatten_and_clauses(expr);
    
    // Sort by selectivity (ascending = most selective first)
    std::sort(and_clauses.begin(), and_clauses.end(),
        [this](const Predicate& a, const Predicate& b) {
            return get_selectivity(a) < get_selectivity(b);
        });
    
    if (and_clauses.empty()) return expr;
    
    // Rebuild expression
    auto result = std::make_shared<Expr>(and_clauses[0]);
    for (size_t i = 1; i < and_clauses.size(); ++i) {
        auto and_expr = std::make_shared<Expr>(ExprType::AND);
        and_expr->left = result;
        and_expr->right = std::make_shared<Expr>(and_clauses[i]);
        result = and_expr;
    }
    
    return result;
}

std::vector<Predicate> QueryRewriter::flatten_and_clauses(const std::shared_ptr<Expr>& expr) {
    std::vector<Predicate> result;
    
    if (!expr) return result;
    
    if (expr->type == ExprType::PREDICATE) {
        result.push_back(expr->pred);
    } else if (expr->type == ExprType::AND) {
        auto left = flatten_and_clauses(expr->left);
        auto right = flatten_and_clauses(expr->right);
        result.insert(result.end(), left.begin(), left.end());
        result.insert(result.end(), right.begin(), right.end());
    }
    
    return result;
}

std::vector<Predicate> QueryRewriter::flatten_or_clauses(const std::shared_ptr<Expr>& expr) {
    std::vector<Predicate> result;
    
    if (!expr) return result;
    
    if (expr->type == ExprType::PREDICATE) {
        result.push_back(expr->pred);
    } else if (expr->type == ExprType::OR) {
        auto left = flatten_or_clauses(expr->left);
        auto right = flatten_or_clauses(expr->right);
        result.insert(result.end(), left.begin(), left.end());
        result.insert(result.end(), right.begin(), right.end());
    }
    
    return result;
}

std::shared_ptr<Expr> QueryRewriter::eliminate_redundant(const std::shared_ptr<Expr>& expr) {
    if (!expr || expr->type != ExprType::AND) {
        return expr;
    }
    
    auto clauses = flatten_and_clauses(expr);
    std::vector<Predicate> kept;
    
    for (size_t i = 0; i < clauses.size(); ++i) {
        bool is_redundant = false;
        
        for (size_t j = 0; j < clauses.size(); ++j) {
            if (i != j && is_dominated(clauses[j], clauses[i])) {
                is_redundant = true;
                break;
            }
        }
        
        if (!is_redundant) {
            kept.push_back(clauses[i]);
        }
    }
    
    if (kept.empty()) return expr;
    
    auto result = std::make_shared<Expr>(kept[0]);
    for (size_t i = 1; i < kept.size(); ++i) {
        auto and_expr = std::make_shared<Expr>(ExprType::AND);
        and_expr->left = result;
        and_expr->right = std::make_shared<Expr>(kept[i]);
        result = and_expr;
    }
    
    return result;
}

bool QueryRewriter::are_contradictory(const Predicate& p1, const Predicate& p2) {
    if (p1.column != p2.column) return false;
    
    // a = 5 AND a = 10
    if (p1.op == CompOp::EQ && p2.op == CompOp::EQ) {
        return p1.value != p2.value;
    }
    
    // a > 100 AND a < 50
    if ((p1.op == CompOp::GT && p2.op == CompOp::LT) ||
        (p1.op == CompOp::LT && p2.op == CompOp::GT)) {
        try {
            double v1 = std::stod(p1.value);
            double v2 = std::stod(p2.value);
            return v1 >= v2;
        } catch (...) {
            return false;
        }
    }
    
    return false;
}

bool QueryRewriter::is_dominated(const Predicate& dominant, const Predicate& redundant) {
    if (dominant.column != redundant.column) return false;
    
    // a > 10 dominates a > 5 (more restrictive)
    if (dominant.op == CompOp::GT && redundant.op == CompOp::GT) {
        try {
            return std::stod(dominant.value) >= std::stod(redundant.value);
        } catch (...) {
            return false;
        }
    }
    
    // a >= 10 dominates a > 5
    if (dominant.op == CompOp::GE && redundant.op == CompOp::GT) {
        try {
            return std::stod(dominant.value) >= std::stod(redundant.value);
        } catch (...) {
            return false;
        }
    }
    
    // a < 10 dominates a < 100
    if (dominant.op == CompOp::LT && redundant.op == CompOp::LT) {
        try {
            return std::stod(dominant.value) <= std::stod(redundant.value);
        } catch (...) {
            return false;
        }
    }
    
    // a = 5 dominates a IN (5, 10, 15)
    if (dominant.op == CompOp::EQ && redundant.op == CompOp::IN) {
        return redundant.value.find(dominant.value) != std::string::npos;
    }
    
    return false;
}

double QueryRewriter::get_selectivity(const Predicate& pred) {
    // Rough selectivity estimates for optimization
    switch (pred.op) {
        case CompOp::EQ:  return 0.01;   // Very selective
        case CompOp::LT:
        case CompOp::GT:  return 0.25;   // Range queries
        case CompOp::LE:
        case CompOp::GE:  return 0.30;   // Inclusive range
        case CompOp::IN:  return 0.10;   // List membership
        case CompOp::NE:
        case CompOp::NIN: return 0.80;   // Not selective
        default:          return 0.50;
    }
}

} // namespace optimization
} // namespace lyradb
