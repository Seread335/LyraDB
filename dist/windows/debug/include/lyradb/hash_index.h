#pragma once

#include <memory>
#include <vector>
#include <functional>
#include <stdexcept>
#include <cstring>
#include <algorithm>

namespace lyradb {
namespace index {

/**
 * @brief Hash-based index for fast equality lookups
 * 
 * Uses hash table with open addressing (linear probing) for O(1) average
 * lookup, insertion, and deletion performance.
 * 
 * Features:
 * - Dynamic resizing with load factor threshold
 * - Linear probing for collision resolution
 * - Support for tombstones to mark deleted entries
 * - Multiple values per key support
 * 
 * Template parameters:
 * - KeyType: Type of index key (must be hashable)
 * - ValueType: Type of indexed values
 */
template<typename KeyType, typename ValueType>
class HashIndex {
public:
    struct Entry {
        KeyType key;
        std::vector<ValueType> values;
        bool tombstone = false;  // Mark deleted entries
        
        Entry() = default;
        Entry(const KeyType& k, const ValueType& v) : key(k), values({v}), tombstone(false) {}
    };
    
    explicit HashIndex(size_t initial_capacity = 1024) 
        : capacity_(initial_capacity), size_(0) {
        table_.resize(capacity_);
    }
    
    ~HashIndex() = default;
    
    /**
     * @brief Insert a key-value pair
     * @param key Index key
     * @param value Value to store
     */
    void insert(const KeyType& key, const ValueType& value) {
        // Resize if load factor exceeds threshold
        if (static_cast<double>(size_) / capacity_ > 0.75) {
            resize();
        }
        
        size_t index = find_or_insert_slot(key);
        
        if (table_[index].values.empty() || table_[index].tombstone) {
            table_[index].key = key;
            table_[index].tombstone = false;
            size_++;
        }
        
        table_[index].values.push_back(value);
    }
    
    /**
     * @brief Search for all values associated with a key
     * @param key Key to search for
     * @return Vector of values, empty if not found
     */
    std::vector<ValueType> search(const KeyType& key) const {
        size_t index = find_slot(key);
        
        if (index != INVALID_INDEX && !table_[index].tombstone) {
            return table_[index].values;
        }
        
        return {};
    }
    
    /**
     * @brief Check if key exists in index
     * @param key Key to check
     * @return True if key found
     */
    bool contains(const KeyType& key) const {
        size_t index = find_slot(key);
        return index != INVALID_INDEX && !table_[index].tombstone;
    }
    
    /**
     * @brief Delete a key-value pair
     * @param key Key to delete
     * @param value Value to delete (if multiple values exist)
     * @return True if deleted
     */
    bool delete_entry(const KeyType& key, const ValueType& value) {
        size_t index = find_slot(key);
        
        if (index == INVALID_INDEX || table_[index].tombstone) {
            return false;
        }
        
        auto& values = table_[index].values;
        auto it = std::find(values.begin(), values.end(), value);
        
        if (it != values.end()) {
            values.erase(it);
            
            // Mark as deleted if no more values
            if (values.empty()) {
                table_[index].tombstone = true;
                size_--;
            }
            
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief Remove all entries with a specific value
     * @param value Value to remove (e.g., row_id)
     * @return Number of entries removed
     */
    size_t remove(const ValueType& value) {
        size_t removed = 0;
        
        for (auto& entry : table_) {
            if (!entry.values.empty() && !entry.tombstone) {
                auto it = std::find(entry.values.begin(), entry.values.end(), value);
                if (it != entry.values.end()) {
                    entry.values.erase(it);
                    removed++;
                    
                    if (entry.values.empty()) {
                        entry.tombstone = true;
                        size_--;
                    }
                }
            }
        }
        
        return removed;
    }
    
    /**
     * @brief Get all entries in the index
     * @return Vector of all (key, values) pairs
     */
    std::vector<std::pair<KeyType, std::vector<ValueType>>> get_all() const {
        std::vector<std::pair<KeyType, std::vector<ValueType>>> result;
        
        for (const auto& entry : table_) {
            if (!entry.values.empty() && !entry.tombstone) {
                result.emplace_back(entry.key, entry.values);
            }
        }
        
        return result;
    }
    
    /**
     * @brief Get number of unique keys
     */
    size_t size() const { return size_; }
    
    /**
     * @brief Check if index is empty
     */
    bool empty() const { return size_ == 0; }
    
    /**
     * @brief Clear the index
     */
    void clear() {
        table_.clear();
        table_.resize(1024);  // Reset to initial size
        capacity_ = 1024;
        size_ = 0;
    }
    
    /**
     * @brief Get current capacity
     */
    size_t capacity() const { return capacity_; }
    
    /**
     * @brief Get load factor
     */
    double load_factor() const {
        return static_cast<double>(size_) / capacity_;
    }

private:
    static constexpr size_t INVALID_INDEX = static_cast<size_t>(-1);
    
    std::vector<Entry> table_;
    size_t capacity_;
    size_t size_;
    
    std::hash<KeyType> hasher_;
    
    // Hash function
    size_t hash(const KeyType& key) const {
        return hasher_(key) % capacity_;
    }
    
    // Find slot for key (linear probing)
    size_t find_slot(const KeyType& key) const {
        size_t index = hash(key);
        size_t attempts = 0;
        
        while (attempts < capacity_) {
            if (table_[index].values.empty() && !table_[index].tombstone) {
                return INVALID_INDEX;  // Key not found
            }
            
            if (!table_[index].tombstone && table_[index].key == key) {
                return index;  // Found
            }
            
            index = (index + 1) % capacity_;
            attempts++;
        }
        
        return INVALID_INDEX;  // Not found after full scan
    }
    
    // Find or insert slot for key
    size_t find_or_insert_slot(const KeyType& key) {
        size_t index = hash(key);
        size_t first_tombstone = INVALID_INDEX;
        size_t attempts = 0;
        
        while (attempts < capacity_) {
            if (table_[index].values.empty() && !table_[index].tombstone) {
                // Empty slot found
                return first_tombstone != INVALID_INDEX ? first_tombstone : index;
            }
            
            if (table_[index].tombstone && first_tombstone == INVALID_INDEX) {
                first_tombstone = index;
            }
            
            if (!table_[index].tombstone && table_[index].key == key) {
                return index;  // Key already exists
            }
            
            index = (index + 1) % capacity_;
            attempts++;
        }
        
        throw std::runtime_error("Hash table is full");
    }
    
    // Resize table when load factor exceeds threshold
    void resize() {
        size_t new_capacity = capacity_ * 2;
        std::vector<Entry> old_table = std::move(table_);
        
        table_.clear();
        table_.resize(new_capacity);
        capacity_ = new_capacity;
        size_ = 0;
        
        // Re-insert all entries
        for (const auto& entry : old_table) {
            if (!entry.values.empty() && !entry.tombstone) {
                for (const auto& value : entry.values) {
                    insert(entry.key, value);
                }
            }
        }
    }
};

} // namespace index
} // namespace lyradb
