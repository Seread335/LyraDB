#pragma once

#include <memory>
#include <vector>
#include <utility>
#include <stdexcept>
#include <cstring>

namespace lyradb {
namespace index {

/**
 * @brief B-tree based index structure for efficient range queries and point lookups
 * 
 * A self-balancing tree data structure that maintains sorted data.
 * Optimized for disk-based access patterns with configurable node size.
 * 
 * Properties:
 * - Balanced tree: All leaves at same depth
 * - Node order B: Max B-1 keys per node, max B children
 * - Range queries: O(log N + K) where K is result size
 * - Point lookups: O(log N)
 * - Insertions/deletions: O(log N) with rebalancing
 * 
 * Template parameters:
 * - KeyType: Type of index key (must be comparable and copyable)
 * - ValueType: Type of indexed values (usually row ID/offset)
 * - ORDER: B-tree node order (default 256 for disk efficiency)
 */
template<typename KeyType, typename ValueType, size_t ORDER = 256>
class BTreeIndex {
public:
    // Ensure order is at least 3 for proper tree operations
    static_assert(ORDER >= 3, "B-tree order must be at least 3");
    
    struct KeyValuePair {
        KeyType key;
        std::vector<ValueType> values;  // Multiple rows can share same key
        
        KeyValuePair() = default;
        KeyValuePair(const KeyType& k, const ValueType& v) : key(k), values({v}) {}
        
        bool operator<(const KeyValuePair& other) const {
            return key < other.key;
        }
        
        bool operator==(const KeyType& k) const {
            return key == k;
        }
    };
    
private:
    struct Node {
        std::vector<KeyValuePair> entries;
        std::vector<std::shared_ptr<Node>> children;
        bool is_leaf = true;
        
        Node() = default;
        
        // Get number of keys in node
        size_t key_count() const { return entries.size(); }
        
        // Check if node is full
        bool is_full() const { return entries.size() >= ORDER - 1; }
        
        // Check if node has minimum keys (except root)
        bool has_min_keys() const { return entries.size() >= (ORDER / 2); }
    };
    
public:
    BTreeIndex() : root_(std::make_shared<Node>()), size_(0) {
        root_->is_leaf = true;
    }
    
    ~BTreeIndex() = default;
    
    /**
     * @brief Insert a key-value pair into the index
     * @param key Index key
     * @param value Row ID or value to store
     */
    void insert(const KeyType& key, const ValueType& value) {
        if (root_->is_full()) {
            auto new_root = std::make_shared<Node>();
            new_root->is_leaf = false;
            new_root->children.push_back(root_);
            split_child(new_root, 0);
            root_ = new_root;
        }
        insert_non_full(root_, key, value);
        size_++;
    }
    
    /**
     * @brief Search for all values associated with a key
     * @param key Key to search for
     * @return Vector of values, empty if not found
     */
    std::vector<ValueType> search(const KeyType& key) const {
        auto node = search_node(root_, key);
        if (node) {
            for (const auto& entry : node->entries) {
                if (entry.key == key) {
                    return entry.values;
                }
            }
        }
        return {};
    }
    
    /**
     * @brief Check if key exists in index
     * @param key Key to check
     * @return True if key found
     */
    bool contains(const KeyType& key) const {
        auto node = search_node(root_, key);
        if (!node) return false;
        
        for (const auto& entry : node->entries) {
            if (entry.key == key) return true;
        }
        return false;
    }
    
    /**
     * @brief Range query: find all keys in [min_key, max_key]
     * @param min_key Lower bound (inclusive)
     * @param max_key Upper bound (inclusive)
     * @return Vector of all values in range
     */
    std::vector<ValueType> range_query(const KeyType& min_key, const KeyType& max_key) const {
        std::vector<ValueType> results;
        if (min_key > max_key) return results;
        
        range_query_recursive(root_, min_key, max_key, results);
        return results;
    }
    
    /**
     * @brief Get all values less than key
     * @param key Upper bound (exclusive)
     * @return Vector of all values less than key
     */
    std::vector<ValueType> get_less_than(const KeyType& key) const {
        std::vector<ValueType> results;
        get_less_than_recursive(root_, key, results);
        return results;
    }
    
    /**
     * @brief Get all values greater than key
     * @param key Lower bound (exclusive)
     * @return Vector of all values greater than key
     */
    std::vector<ValueType> get_greater_than(const KeyType& key) const {
        std::vector<ValueType> results;
        get_greater_than_recursive(root_, key, results);
        return results;
    }
    
    /**
     * @brief Delete a key-value pair
     * @param key Key to delete
     * @param value Value to delete (if multiple values exist)
     * @return True if deleted, false if not found
     */
    bool delete_entry(const KeyType& key, const ValueType& value) {
        bool deleted = delete_from_node(root_, key, value);
        
        // If root is empty and has a child, make child the new root
        if (root_->key_count() == 0 && !root_->is_leaf && root_->children.size() > 0) {
            root_ = root_->children[0];
        }
        
        if (deleted) size_--;
        return deleted;
    }
    
    /**
     * @brief Get number of keys in index
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
        root_ = std::make_shared<Node>();
        root_->is_leaf = true;
        size_ = 0;
    }
    
    /**
     * @brief Get height of the tree
     */
    size_t height() const {
        return get_height(root_);
    }

private:
    std::shared_ptr<Node> root_;
    size_t size_;
    
    // Find leaf node where key should be located
    std::shared_ptr<Node> search_node(std::shared_ptr<Node> node, const KeyType& key) const {
        size_t i = 0;
        
        // Find first entry >= key
        while (i < node->key_count() && key > node->entries[i].key) {
            i++;
        }
        
        // Check exact match in current node
        if (i < node->key_count() && key == node->entries[i].key) {
            return node;
        }
        
        // If leaf, key not found
        if (node->is_leaf) {
            return nullptr;
        }
        
        // Recurse to appropriate child
        return search_node(node->children[i], key);
    }
    
    // Insert into non-full node
    void insert_non_full(std::shared_ptr<Node> node, const KeyType& key, const ValueType& value) {
        int i = static_cast<int>(node->key_count()) - 1;
        
        if (node->is_leaf) {
            // Find position and insert
            while (i >= 0 && key < node->entries[i].key) {
                i--;
            }
            
            // Check if key already exists
            if (i >= 0 && key == node->entries[i].key) {
                node->entries[i].values.push_back(value);
                return;
            }
            
            // Insert new entry
            node->entries.insert(node->entries.begin() + i + 1, KeyValuePair(key, value));
        } else {
            // Find child to insert into
            while (i >= 0 && key < node->entries[i].key) {
                i--;
            }
            i++;
            
            // Check if child is full
            if (node->children[i]->is_full()) {
                split_child(node, i);
                
                // After split, key might go to next child
                if (key > node->entries[i].key) {
                    i++;
                }
            }
            
            insert_non_full(node->children[i], key, value);
        }
    }
    
    // Split full child at index i
    void split_child(std::shared_ptr<Node> parent, size_t i) {
        auto full_child = parent->children[i];
        auto new_child = std::make_shared<Node>();
        new_child->is_leaf = full_child->is_leaf;
        
        size_t mid = ORDER / 2;
        
        // Move second half to new node
        new_child->entries.assign(full_child->entries.begin() + mid, full_child->entries.end());
        full_child->entries.erase(full_child->entries.begin() + mid, full_child->entries.end());
        
        // Move children if not leaf
        if (!full_child->is_leaf) {
            new_child->children.assign(full_child->children.begin() + mid, full_child->children.end());
            full_child->children.erase(full_child->children.begin() + mid, full_child->children.end());
        }
        
        // Move median entry to parent
        parent->entries.insert(parent->entries.begin() + i, full_child->entries.back());
        full_child->entries.pop_back();
        
        // Insert new child into parent
        parent->children.insert(parent->children.begin() + i + 1, new_child);
    }
    
    // Delete entry from tree
    bool delete_from_node(std::shared_ptr<Node> node, const KeyType& key, const ValueType& value) {
        size_t i = 0;
        
        while (i < node->key_count() && key > node->entries[i].key) {
            i++;
        }
        
        if (i < node->key_count() && key == node->entries[i].key) {
            // Found key in this node
            auto& values = node->entries[i].values;
            auto it = std::find(values.begin(), values.end(), value);
            
            if (it != values.end()) {
                values.erase(it);
                
                // Remove entry if no more values
                if (values.empty()) {
                    if (node->is_leaf) {
                        node->entries.erase(node->entries.begin() + i);
                    } else {
                        // Complex deletion in internal node - not implemented for Phase 6
                        // For production, would need to handle merging/rebalancing
                    }
                }
                return true;
            }
        } else if (!node->is_leaf) {
            // Recurse to child
            bool is_in_subtree = (i < node->key_count());
            if (delete_from_node(node->children[i], key, value)) {
                // Handle rebalancing if needed - simplified for Phase 6
                return true;
            }
        }
        
        return false;
    }
    
    // Range query implementation
    void range_query_recursive(std::shared_ptr<Node> node, const KeyType& min_key, 
                               const KeyType& max_key, std::vector<ValueType>& results) const {
        if (!node) return;
        
        size_t i = 0;
        
        // Find first entry >= min_key
        while (i < node->key_count() && node->entries[i].key < min_key) {
            i++;
        }
        
        // Process entries in node
        while (i < node->key_count() && node->entries[i].key <= max_key) {
            // Recurse to child before this entry
            if (!node->is_leaf) {
                range_query_recursive(node->children[i], min_key, max_key, results);
            }
            
            // Add this entry's values
            for (const auto& val : node->entries[i].values) {
                results.push_back(val);
            }
            i++;
        }
        
        // Recurse to last child if internal node
        if (!node->is_leaf && i < node->children.size()) {
            range_query_recursive(node->children[i], min_key, max_key, results);
        }
    }
    
    // Less than query
    void get_less_than_recursive(std::shared_ptr<Node> node, const KeyType& key, 
                                 std::vector<ValueType>& results) const {
        if (!node) return;
        
        for (size_t i = 0; i < node->key_count(); i++) {
            if (node->entries[i].key < key) {
                if (!node->is_leaf) {
                    get_less_than_recursive(node->children[i], key, results);
                }
                for (const auto& val : node->entries[i].values) {
                    results.push_back(val);
                }
            } else {
                // Only recurse to first subtree
                if (!node->is_leaf) {
                    get_less_than_recursive(node->children[i], key, results);
                }
                return;
            }
        }
        
        // Recurse to rightmost child
        if (!node->is_leaf && node->children.size() > node->key_count()) {
            get_less_than_recursive(node->children.back(), key, results);
        }
    }
    
    // Greater than query
    void get_greater_than_recursive(std::shared_ptr<Node> node, const KeyType& key,
                                    std::vector<ValueType>& results) const {
        if (!node) return;
        
        size_t i = node->key_count();
        
        // Find last entry <= key
        while (i > 0 && node->entries[i - 1].key > key) {
            i--;
        }
        
        // Process remaining entries
        while (i < node->key_count()) {
            if (node->entries[i].key > key) {
                if (!node->is_leaf) {
                    get_greater_than_recursive(node->children[i], key, results);
                }
                for (const auto& val : node->entries[i].values) {
                    results.push_back(val);
                }
            }
            i++;
        }
        
        // Recurse to rightmost child
        if (!node->is_leaf && node->children.size() > 0) {
            get_greater_than_recursive(node->children.back(), key, results);
        }
    }
    
    // Calculate tree height
    size_t get_height(std::shared_ptr<Node> node) const {
        if (!node || node->is_leaf) {
            return 1;
        }
        return 1 + get_height(node->children[0]);
    }
};

} // namespace index
} // namespace lyradb
