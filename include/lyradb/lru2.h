#pragma once

#include <list>
#include <unordered_map>
#include <stdexcept>

namespace lyradb {

/**
 * @brief LRU2 (Two-level LRU) replacement policy
 * Improved over standard LRU with frequency consideration
 * 
 * Pages are promoted through two levels:
 * - Probation: Recently accessed once
 * - Protected: Recently accessed multiple times
 */
template<typename Key>
class LRU2 {
public:
    explicit LRU2(size_t capacity) : capacity_(capacity) {}
    
    /**
     * @brief Record access to a key
     */
    void access(const Key& key) {
        auto it = key_location_.find(key);
        if (it == key_location_.end()) {
            // New key - add to probation list
            probation_.push_front(key);
            key_location_[key] = {ProbationList, probation_.begin()};
        } else {
            auto [level, iter] = it->second;
            if (level == ProbationList) {
                // Promoted from probation to protected
                probation_.erase(iter);
                protected_.push_front(key);
                key_location_[key] = {ProtectedList, protected_.begin()};
            } else {
                // Already in protected list, move to front
                protected_.erase(iter);
                protected_.push_front(key);
                key_location_[key] = {ProtectedList, protected_.begin()};
            }
        }
        
        // Evict if necessary
        if (key_location_.size() > capacity_) {
            evict();
        }
    }
    
    /**
     * @brief Get LRU candidate for eviction
     */
    Key evict_candidate() {
        if (!probation_.empty()) {
            return probation_.back();
        } else if (!protected_.empty()) {
            return protected_.back();
        } else {
            throw std::runtime_error("LRU2 is empty");
        }
    }
    
    void clear() {
        probation_.clear();
        protected_.clear();
        key_location_.clear();
    }
    
    size_t size() const { return key_location_.size(); }
    
private:
    enum Level { ProbationList, ProtectedList };
    
    using ListType = std::list<Key>;
    using IterType = typename ListType::iterator;
    using LocationType = std::pair<Level, IterType>;
    
    size_t capacity_;
    ListType probation_;
    ListType protected_;
    std::unordered_map<Key, LocationType> key_location_;
    
    void evict() {
        Key victim = evict_candidate();
        if (!probation_.empty() && probation_.back() == victim) {
            probation_.pop_back();
        } else if (!protected_.empty() && protected_.back() == victim) {
            protected_.pop_back();
        }
        key_location_.erase(victim);
    }
};

} // namespace lyradb
