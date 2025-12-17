/**
 * @file b_tree.h
 * @brief B-tree template implementation for range queries
 * 
 * Phase 4.2: B-Tree Index Implementation
 * 
 * Supports:
 * - Range queries: >, <, >=, <=, BETWEEN
 * - Single and multi-column keys
 * - O(log n) search, insert, delete
 * - Automatic balancing
 * 
 * Template Parameters:
 * - KeyType: Key type (string, CompositeKey, etc.)
 * - ValueType: Value type (row ID, etc.)
 */

#pragma once

#include <vector>
#include <memory>
#include <algorithm>
#include <stdexcept>

namespace lyradb {
namespace index {

/**
 * @brief B-tree node
 * @tparam KeyType Key type
 * @tparam ValueType Value type
 */
template <typename KeyType, typename ValueType>
class BTreeNode {
public:
    static constexpr int MIN_DEGREE = 4;  // Minimum degree (t)
    static constexpr int MAX_KEYS = 2 * MIN_DEGREE - 1;
    static constexpr int MAX_CHILDREN = 2 * MIN_DEGREE;

    // Public data members (for simplicity)
    std::vector<KeyType> keys;
    std::vector<ValueType> values;  // Leaf node only
    std::vector<std::shared_ptr<BTreeNode>> children;  // Internal node only
    bool is_leaf = true;
    int num_keys = 0;

    BTreeNode() : keys(MAX_KEYS), values(MAX_KEYS), children(MAX_CHILDREN) {}

    /**
     * @brief Search for key in this node and subtree
     * @param key Search key
     * @return Vector of values matching the key
     */
    std::vector<ValueType> search(const KeyType& key) const {
        std::vector<ValueType> result;
        
        // Find position of key
        int idx = 0;
        while (idx < num_keys && key > keys[idx]) {
            idx++;
        }
        
        // Check if key found in this node
        if (idx < num_keys && key == keys[idx]) {
            if (is_leaf) {
                result.push_back(values[idx]);
            }
            return result;
        }
        
        // If leaf, key not found
        if (is_leaf) {
            return result;
        }
        
        // Recursively search in appropriate child
        if (children[idx]) {
            return children[idx]->search(key);
        }
        
        return result;
    }

    /**
     * @brief Range search: all keys >= min_key and <= max_key
     * @param min_key Minimum key (inclusive)
     * @param max_key Maximum key (inclusive)
     * @return Vector of all matching values
     */
    std::vector<ValueType> range_search(const KeyType& min_key, const KeyType& max_key) const {
        std::vector<ValueType> result;
        range_search_impl(min_key, max_key, result);
        return result;
    }

    /**
     * @brief Insert key-value pair into leaf node
     * @param key Key to insert
     * @param value Value to insert
     */
    void insert_leaf(const KeyType& key, const ValueType& value) {
        if (!is_leaf) {
            throw std::runtime_error("insert_leaf called on non-leaf node");
        }

        // Find position to insert
        int idx = 0;
        while (idx < num_keys && key > keys[idx]) {
            idx++;
        }

        // Shift keys and values right
        for (int i = num_keys; i > idx; --i) {
            keys[i] = keys[i - 1];
            values[i] = values[i - 1];
        }

        // Insert new key-value
        keys[idx] = key;
        values[idx] = value;
        num_keys++;
    }

    /**
     * @brief Check if node is full
     * @return true if node has max keys
     */
    bool is_full() const {
        return num_keys == MAX_KEYS;
    }

    /**
     * @brief Check if node has minimum keys
     * @return true if num_keys == MIN_DEGREE - 1
     */
    bool has_min_keys() const {
        return num_keys == MIN_DEGREE - 1;
    }

private:
    /**
     * @brief Recursive implementation of range search
     */
    void range_search_impl(const KeyType& min_key, const KeyType& max_key,
                           std::vector<ValueType>& result) const {
        int idx = 0;

        // Find first position >= min_key
        while (idx < num_keys && keys[idx] < min_key) {
            idx++;
        }

        // Process keys in current node
        while (idx < num_keys && keys[idx] <= max_key) {
            if (keys[idx] >= min_key) {
                if (is_leaf) {
                    result.push_back(values[idx]);
                }
            }
            idx++;
        }

        // Recursively search children if not leaf
        if (!is_leaf) {
            idx = 0;
            while (idx < num_keys && keys[idx] < min_key) {
                idx++;
            }

            // Search all relevant children
            while (idx <= num_keys && (idx == 0 || keys[idx - 1] < max_key)) {
                if (children[idx]) {
                    children[idx]->range_search_impl(min_key, max_key, result);
                }
                idx++;
            }
        }
    }
};

/**
 * @brief B-tree template class
 * @tparam KeyType Key type (must support <, >, ==, comparisons)
 * @tparam ValueType Value type (typically size_t for row IDs)
 */
template <typename KeyType, typename ValueType>
class BTree {
public:
    using Node = BTreeNode<KeyType, ValueType>;

    BTree() : root_(std::make_shared<Node>()) {
        root_->is_leaf = true;
    }

    /**
     * @brief Search for exact key
     * @param key Search key
     * @return Vector of values matching the key (usually one)
     */
    std::vector<ValueType> search(const KeyType& key) const {
        return root_->search(key);
    }

    /**
     * @brief Range search: all keys >= min_key and <= max_key
     * @param min_key Minimum key (inclusive)
     * @param max_key Maximum key (inclusive)
     * @return Vector of all matching values
     */
    std::vector<ValueType> range_search(const KeyType& min_key, const KeyType& max_key) const {
        return root_->range_search(min_key, max_key);
    }

    /**
     * @brief Insert key-value pair
     * @param key Key to insert
     * @param value Value to insert
     */
    void insert(const KeyType& key, const ValueType& value) {
        if (root_->is_full()) {
            // Create new root
            auto new_root = std::make_shared<Node>();
            new_root->is_leaf = false;
            new_root->children[0] = root_;
            split_child(new_root, 0);
            root_ = new_root;
        }

        insert_non_full(root_, key, value);
    }

    /**
     * @brief Delete a key-value pair
     * @param key Key to delete
     * @note Current implementation removes one occurrence
     */
    void delete_key(const KeyType& key) {
        delete_internal(root_, key);

        // If root is empty after deletion, make its only child the new root
        if (root_->num_keys == 0) {
            if (!root_->is_leaf && root_->children[0]) {
                root_ = root_->children[0];
            }
        }
    }

    /**
     * @brief Get root node (for testing)
     * @return Shared pointer to root node
     */
    std::shared_ptr<Node> get_root() const {
        return root_;
    }

private:
    std::shared_ptr<Node> root_;

    /**
     * @brief Split a full child of a parent node
     */
    void split_child(std::shared_ptr<Node> parent, int idx) {
        auto full_child = parent->children[idx];
        auto new_child = std::make_shared<Node>();
        new_child->is_leaf = full_child->is_leaf;

        int mid = Node::MIN_DEGREE - 1;

        // Copy right half to new node
        for (int i = 0; i < mid; ++i) {
            new_child->keys[i] = full_child->keys[i + mid + 1];
            if (full_child->is_leaf) {
                new_child->values[i] = full_child->values[i + mid + 1];
            }
        }

        // Copy child pointers if internal node
        if (!full_child->is_leaf) {
            for (int i = 0; i <= mid; ++i) {
                new_child->children[i] = full_child->children[i + mid + 1];
            }
        }

        new_child->num_keys = mid;
        full_child->num_keys = mid;

        // Move middle key up to parent
        for (int i = parent->num_keys; i > idx; --i) {
            parent->keys[i] = parent->keys[i - 1];
            parent->children[i + 1] = parent->children[i];
        }

        parent->keys[idx] = full_child->keys[mid];
        parent->children[idx + 1] = new_child;
        parent->num_keys++;
    }

    /**
     * @brief Insert into non-full node
     */
    void insert_non_full(std::shared_ptr<Node> node, const KeyType& key, const ValueType& value) {
        int idx = node->num_keys - 1;

        if (node->is_leaf) {
            node->insert_leaf(key, value);
        } else {
            // Find child to descend into
            while (idx >= 0 && key < node->keys[idx]) {
                idx--;
            }
            idx++;

            auto child = node->children[idx];
            if (child->is_full()) {
                split_child(node, idx);
                if (key > node->keys[idx]) {
                    idx++;
                }
            }

            insert_non_full(node->children[idx], key, value);
        }
    }

    /**
     * @brief Delete a key from the tree
     */
    void delete_internal(std::shared_ptr<Node> node, const KeyType& key) {
        int idx = 0;

        // Find position
        while (idx < node->num_keys && key > node->keys[idx]) {
            idx++;
        }

        if (node->is_leaf) {
            // Delete from leaf
            if (idx < node->num_keys && key == node->keys[idx]) {
                for (int i = idx; i < node->num_keys - 1; ++i) {
                    node->keys[i] = node->keys[i + 1];
                    node->values[i] = node->values[i + 1];
                }
                node->num_keys--;
            }
        } else {
            // Delete from internal node
            if (idx < node->num_keys && key == node->keys[idx]) {
                // Key found in internal node
                if (node->children[idx]->num_keys >= Node::MIN_DEGREE) {
                    // Predecessor has enough keys
                    auto pred = get_predecessor(node, idx);
                    node->keys[idx] = pred;
                    delete_internal(node->children[idx], pred);
                } else {
                    // Merge with sibling
                    merge(node, idx);
                    delete_internal(node->children[idx], key);
                }
            } else {
                // Key not in this node, recurse
                bool is_in_subtree = (idx == node->num_keys);

                if (node->children[idx]->num_keys < Node::MIN_DEGREE) {
                    // Child has minimum keys, may need to prepare
                    fill_child(node, idx);
                }

                if (is_in_subtree && idx > node->num_keys) {
                    delete_internal(node->children[idx - 1], key);
                } else {
                    delete_internal(node->children[idx], key);
                }
            }
        }
    }

    /**
     * @brief Get predecessor key from subtree
     */
    KeyType get_predecessor(std::shared_ptr<Node> node, int idx) {
        auto child = node->children[idx];
        while (!child->is_leaf) {
            child = child->children[child->num_keys];
        }
        return child->keys[child->num_keys - 1];
    }

    /**
     * @brief Fill child if it has minimum keys
     */
    void fill_child(std::shared_ptr<Node> parent, int idx) {
        auto child = parent->children[idx];

        // Borrow from left sibling
        if (idx != 0 && parent->children[idx - 1]->num_keys >= Node::MIN_DEGREE) {
            borrow_from_left(parent, idx);
        }
        // Borrow from right sibling
        else if (idx != parent->num_keys && parent->children[idx + 1]->num_keys >= Node::MIN_DEGREE) {
            borrow_from_right(parent, idx);
        }
        // Merge with sibling
        else {
            if (idx != parent->num_keys) {
                merge(parent, idx);
            } else {
                merge(parent, idx - 1);
            }
        }
    }

    /**
     * @brief Borrow key from left sibling
     */
    void borrow_from_left(std::shared_ptr<Node> parent, int child_idx) {
        auto child = parent->children[child_idx];
        auto left_sibling = parent->children[child_idx - 1];

        // Move key from parent to child
        for (int i = child->num_keys; i > 0; --i) {
            child->keys[i] = child->keys[i - 1];
            if (child->is_leaf) {
                child->values[i] = child->values[i - 1];
            }
        }

        child->keys[0] = parent->keys[child_idx - 1];
        if (child->is_leaf) {
            child->values[0] = left_sibling->values[left_sibling->num_keys - 1];
        } else {
            child->children[0] = left_sibling->children[left_sibling->num_keys];
        }

        // Move key from left sibling to parent
        parent->keys[child_idx - 1] = left_sibling->keys[left_sibling->num_keys - 1];

        child->num_keys++;
        left_sibling->num_keys--;
    }

    /**
     * @brief Borrow key from right sibling
     */
    void borrow_from_right(std::shared_ptr<Node> parent, int child_idx) {
        auto child = parent->children[child_idx];
        auto right_sibling = parent->children[child_idx + 1];

        // Move key from parent to child
        child->keys[child->num_keys] = parent->keys[child_idx];
        if (child->is_leaf) {
            child->values[child->num_keys] = right_sibling->values[0];
        } else {
            child->children[child->num_keys + 1] = right_sibling->children[0];
        }

        // Move key from right sibling to parent
        parent->keys[child_idx] = right_sibling->keys[0];

        // Remove key from right sibling
        for (int i = 0; i < right_sibling->num_keys - 1; ++i) {
            right_sibling->keys[i] = right_sibling->keys[i + 1];
            if (right_sibling->is_leaf) {
                right_sibling->values[i] = right_sibling->values[i + 1];
            }
        }

        if (!right_sibling->is_leaf) {
            for (int i = 0; i < right_sibling->num_keys; ++i) {
                right_sibling->children[i] = right_sibling->children[i + 1];
            }
        }

        child->num_keys++;
        right_sibling->num_keys--;
    }

    /**
     * @brief Merge child with its sibling
     */
    void merge(std::shared_ptr<Node> parent, int idx) {
        auto child = parent->children[idx];
        auto right_sibling = parent->children[idx + 1];

        // Copy key from parent and right sibling to child
        child->keys[Node::MIN_DEGREE - 1] = parent->keys[idx];

        for (int i = 0; i < right_sibling->num_keys; ++i) {
            child->keys[i + Node::MIN_DEGREE] = right_sibling->keys[i];
            if (child->is_leaf) {
                child->values[i + Node::MIN_DEGREE] = right_sibling->values[i];
            }
        }

        if (!child->is_leaf) {
            for (int i = 0; i <= right_sibling->num_keys; ++i) {
                child->children[i + Node::MIN_DEGREE] = right_sibling->children[i];
            }
        }

        // Move keys in parent
        for (int i = idx; i < parent->num_keys - 1; ++i) {
            parent->keys[i] = parent->keys[i + 1];
            parent->children[i + 1] = parent->children[i + 2];
        }

        child->num_keys += right_sibling->num_keys + 1;
        parent->num_keys--;
    }
};

} // namespace index
} // namespace lyradb
