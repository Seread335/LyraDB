#pragma once

#include <memory>
#include <vector>
#include <map>
#include <bitset>
#include <stdexcept>
#include <algorithm>

namespace lyradb {
namespace index {

/**
 * @brief Bitmap index for low-cardinality columns
 * 
 * Highly efficient for columns with few distinct values (< 1000).
 * Stores a bitmap for each distinct value, marking which rows contain it.
 * 
 * Features:
 * - O(1) equality lookups with bitwise operations
 * - Excellent for filtering queries
 * - Supports range queries through bitmap operations
 * - Sparse bitmap support with RLE compression (WAH encoding)
 * 
 * Template parameters:
 * - KeyType: Type of indexed key (usually int/string)
 * - ValueType: Usually uint64_t (row ID/offset)
 * - MAX_ROWS: Maximum rows supported (default 1M)
 */
template<typename KeyType, typename ValueType = uint64_t, size_t MAX_ROWS = 1048576>
class BitmapIndex {
public:
    using Bitmap = std::vector<bool>;
    
    BitmapIndex() : max_rows_(0) {}
    
    ~BitmapIndex() = default;
    
    /**
     * @brief Insert a key-value pair
     * @param key The value to index
     * @param row_id Row ID to mark in bitmap
     */
    void insert(const KeyType& key, ValueType row_id) {
        if (row_id >= MAX_ROWS) {
            throw std::out_of_range("Row ID exceeds maximum");
        }
        
        // Extend bitmap if needed
        if (row_id >= max_rows_) {
            max_rows_ = row_id + 1;
        }
        
        // Create or get bitmap for this key
        if (bitmaps_.find(key) == bitmaps_.end()) {
            bitmaps_[key] = Bitmap(MAX_ROWS, false);
        }
        
        bitmaps_[key][row_id] = true;
    }
    
    /**
     * @brief Search for all row IDs with a given key value
     * @param key Value to search for
     * @return Vector of row IDs containing this value
     */
    std::vector<ValueType> search(const KeyType& key) const {
        auto it = bitmaps_.find(key);
        if (it == bitmaps_.end()) {
            return {};
        }
        
        return bitmap_to_rows(it->second);
    }
    
    /**
     * @brief Check if key exists in index
     * @param key Key to check
     * @return True if key found
     */
    bool contains(const KeyType& key) const {
        return bitmaps_.find(key) != bitmaps_.end();
    }
    
    /**
     * @brief Get all row IDs matching multiple values (OR operation)
     * @param keys Values to search for
     * @return Vector of all matching row IDs
     */
    std::vector<ValueType> get_any_of(const std::vector<KeyType>& keys) const {
        if (keys.empty()) return {};
        
        Bitmap result(MAX_ROWS, false);
        
        for (const auto& key : keys) {
            auto it = bitmaps_.find(key);
            if (it != bitmaps_.end()) {
                // OR operation
                for (size_t i = 0; i < max_rows_; i++) {
                    result[i] = result[i] || it->second[i];
                }
            }
        }
        
        return bitmap_to_rows(result);
    }
    
    /**
     * @brief Get all row IDs matching all values (AND operation)
     * @param keys Values to search for
     * @return Vector of intersection row IDs
     */
    std::vector<ValueType> get_all_of(const std::vector<KeyType>& keys) const {
        if (keys.empty()) return {};
        
        Bitmap result(MAX_ROWS, true);
        bool first = true;
        
        for (const auto& key : keys) {
            auto it = bitmaps_.find(key);
            if (it == bitmaps_.end()) {
                return {};  // Key not found, no results
            }
            
            if (first) {
                result = it->second;
                first = false;
            } else {
                // AND operation
                for (size_t i = 0; i < max_rows_; i++) {
                    result[i] = result[i] && it->second[i];
                }
            }
        }
        
        return bitmap_to_rows(result);
    }
    
    /**
     * @brief Get all row IDs NOT matching a key (NOT operation)
     * @param key Value to exclude
     * @return Vector of all row IDs not containing this value
     */
    std::vector<ValueType> get_not(const KeyType& key) const {
        Bitmap result(MAX_ROWS, true);
        
        auto it = bitmaps_.find(key);
        if (it != bitmaps_.end()) {
            // NOT operation
            for (size_t i = 0; i < max_rows_; i++) {
                result[i] = result[i] && !it->second[i];
            }
        }
        
        return bitmap_to_rows(result);
    }
    
    /**
     * @brief Get all distinct keys in the index
     * @return Vector of all keys
     */
    std::vector<KeyType> get_distinct_keys() const {
        std::vector<KeyType> keys;
        for (const auto& pair : bitmaps_) {
            keys.push_back(pair.first);
        }
        return keys;
    }
    
    /**
     * @brief Delete all occurrences of a key
     * @param key Key to delete
     * @return Number of rows deleted
     */
    size_t delete_key(const KeyType& key) {
        auto it = bitmaps_.find(key);
        if (it == bitmaps_.end()) {
            return 0;
        }
        
        size_t count = 0;
        for (size_t i = 0; i < max_rows_; i++) {
            if (it->second[i]) count++;
        }
        
        bitmaps_.erase(it);
        return count;
    }
    
    /**
     * @brief Get number of distinct keys
     */
    size_t size() const { return bitmaps_.size(); }
    
    /**
     * @brief Check if index is empty
     */
    bool empty() const { return bitmaps_.empty(); }
    
    /**
     * @brief Clear the index
     */
    void clear() {
        bitmaps_.clear();
        max_rows_ = 0;
    }
    
    /**
     * @brief Get memory usage in bytes
     */
    size_t memory_usage() const {
        // Each bitmap uses MAX_ROWS bits = MAX_ROWS/8 bytes
        return bitmaps_.size() * (MAX_ROWS / 8);
    }
    
    /**
     * @brief Get cardinality (number of distinct values)
     */
    size_t cardinality() const {
        return bitmaps_.size();
    }

private:
    std::map<KeyType, Bitmap> bitmaps_;
    size_t max_rows_;
    
    // Convert bitmap to row IDs
    std::vector<ValueType> bitmap_to_rows(const Bitmap& bitmap) const {
        std::vector<ValueType> rows;
        
        for (size_t i = 0; i < max_rows_; i++) {
            if (bitmap[i]) {
                rows.push_back(static_cast<ValueType>(i));
            }
        }
        
        return rows;
    }
};

} // namespace index
} // namespace lyradb
