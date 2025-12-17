#pragma once

#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <cstring>

namespace lyradb {
namespace index {

/**
 * @class CompositeKey
 * @brief Tuple-like key for multi-column hash indexes
 * 
 * Combines multiple string values into a single hashable key.
 * Uses string concatenation with delimiters for hashing.
 * 
 * Phase 4.1.2: Multi-column index support
 */
class CompositeKey {
public:
    CompositeKey() = default;
    
    explicit CompositeKey(const std::vector<std::string>& values) 
        : values_(values), cached_hash_(0), hash_computed_(false) {
    }
    
    // Construction from variadic arguments
    template<typename... Args>
    explicit CompositeKey(const Args&... args) 
        : cached_hash_(0), hash_computed_(false) {
        (add_value(args), ...);
    }
    
    // Add a value to the composite key
    void add_value(const std::string& value) {
        values_.push_back(value);
        hash_computed_ = false;
    }
    
    // Add a numeric value (converted to string)
    void add_value(int64_t value) {
        values_.push_back(std::to_string(value));
        hash_computed_ = false;
    }
    
    // Add a numeric value (converted to string)
    void add_value(int value) {
        values_.push_back(std::to_string(value));
        hash_computed_ = false;
    }
    
    // Get number of columns in composite key
    size_t size() const {
        return values_.size();
    }
    
    // Get column value at index
    const std::string& get(size_t index) const {
        if (index >= values_.size()) {
            throw std::out_of_range("CompositeKey index out of range");
        }
        return values_[index];
    }
    
    // Get all values
    const std::vector<std::string>& values() const {
        return values_;
    }
    
    // Convert to string representation for debugging
    std::string to_string() const {
        std::stringstream ss;
        ss << "(";
        for (size_t i = 0; i < values_.size(); ++i) {
            if (i > 0) ss << ",";
            ss << values_[i];
        }
        ss << ")";
        return ss.str();
    }
    
    // Equality comparison
    bool operator==(const CompositeKey& other) const {
        return values_ == other.values_;
    }
    
    // Inequality comparison
    bool operator!=(const CompositeKey& other) const {
        return !(*this == other);
    }
    
    // Less than comparison (lexicographic)
    bool operator<(const CompositeKey& other) const {
        return values_ < other.values_;
    }
    
    // Greater than comparison (lexicographic)
    bool operator>(const CompositeKey& other) const {
        return values_ > other.values_;
    }
    
    // Less than or equal comparison (lexicographic)
    bool operator<=(const CompositeKey& other) const {
        return values_ <= other.values_;
    }
    
    // Greater than or equal comparison (lexicographic)
    bool operator>=(const CompositeKey& other) const {
        return values_ >= other.values_;
    }
    
    // Hash function for use in std::unordered_map
    size_t hash() const {
        if (!hash_computed_) {
            compute_hash();
        }
        return cached_hash_;
    }

private:
    std::vector<std::string> values_;
    mutable size_t cached_hash_;
    mutable bool hash_computed_;
    
    static constexpr size_t DELIMITER = 0x1F;  // Use ASCII Unit Separator as delimiter
    
    void compute_hash() const {
        // Combine hashes of all values using FNV-1a algorithm
        constexpr size_t FNV_offset_basis = 14695981039346656037ULL;
        constexpr size_t FNV_prime = 1099511628211ULL;
        
        cached_hash_ = FNV_offset_basis;
        
        for (const auto& value : values_) {
            // Hash the delimiter
            cached_hash_ ^= DELIMITER;
            cached_hash_ *= FNV_prime;
            
            // Hash each byte of the value
            for (unsigned char byte : value) {
                cached_hash_ ^= byte;
                cached_hash_ *= FNV_prime;
            }
        }
        
        hash_computed_ = true;
    }
};

} // namespace index
} // namespace lyradb

// Specialization for std::hash
namespace std {
    template<>
    struct hash<lyradb::index::CompositeKey> {
        size_t operator()(const lyradb::index::CompositeKey& key) const {
            return key.hash();
        }
    };
}
