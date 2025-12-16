#include "lyradb/query_cache.h"
#include <functional>
#include <algorithm>
#include <sstream>

namespace lyradb {

QueryCache::QueryCache(size_t max_entries, int64_t ttl_seconds, size_t max_bytes)
    : max_entries_(max_entries), ttl_seconds_(ttl_seconds), max_bytes_(max_bytes) {
}

std::string QueryCache::compute_cache_key(const std::string& query_sql) const {
    // Simple hash-based key from SQL text
    // TODO: Normalize SQL before hashing (remove whitespace, comments)
    std::hash<std::string> hasher;
    return std::to_string(hasher(query_sql));
}

std::shared_ptr<QueryResult> QueryCache::get(const std::string& query_sql) {
    if (!enabled_) {
        stats_.total_misses++;
        return nullptr;
    }
    
    std::string key = compute_cache_key(query_sql);
    auto it = cache_data_.find(key);
    
    if (it != cache_data_.end()) {
        CacheEntry& entry = it->second;
        
        // Check if entry has expired
        if (entry.is_expired(ttl_seconds_)) {
            cache_data_.erase(it);
            stats_.total_misses++;
            return nullptr;
        }
        
        // Cache hit - update LRU order
        auto order_it = std::find(access_order_.begin(), access_order_.end(), key);
        if (order_it != access_order_.end()) {
            access_order_.erase(order_it);
        }
        access_order_.push_back(key);  // Move to end (most recent)
        
        stats_.total_hits++;
        return entry.result;
    }
    
    stats_.total_misses++;
    return nullptr;
}

void QueryCache::put(const std::string& query_sql,
                     std::shared_ptr<QueryResult> result,
                     const std::set<std::string>& affected_tables) {
    if (!enabled_ || !result) {
        return;
    }
    
    std::string key = compute_cache_key(query_sql);
    size_t result_size = estimate_result_size(result.get());
    
    // Check if adding this would exceed memory limit
    size_t current_used = 0;
    for (const auto& [k, entry] : cache_data_) {
        current_used += entry.bytes_used;
    }
    
    // Evict entries if necessary
    while (cache_data_.size() >= max_entries_ || 
           (current_used + result_size > max_bytes_ && !cache_data_.empty())) {
        evict_lru();
        current_used = 0;
        for (const auto& [k, entry] : cache_data_) {
            current_used += entry.bytes_used;
        }
    }
    
    // Create new cache entry
    CacheEntry entry;
    entry.result = result;
    entry.affected_tables = affected_tables;
    entry.bytes_used = result_size;
    entry.created_at = std::chrono::steady_clock::now();
    
    // Store in cache
    cache_data_[key] = entry;
    access_order_.push_back(key);
    
    // Track table to query mapping for invalidation
    for (const auto& table_name : affected_tables) {
        table_to_queries_[table_name].insert(key);
    }
}

size_t QueryCache::invalidate(const std::string& table_name) {
    size_t invalidated_count = 0;
    
    auto table_it = table_to_queries_.find(table_name);
    if (table_it != table_to_queries_.end()) {
        for (const auto& key : table_it->second) {
            auto cache_it = cache_data_.find(key);
            if (cache_it != cache_data_.end()) {
                cache_data_.erase(cache_it);
                invalidated_count++;
            }
            
            // Remove from access order
            auto order_it = std::find(access_order_.begin(), access_order_.end(), key);
            if (order_it != access_order_.end()) {
                access_order_.erase(order_it);
            }
        }
        table_to_queries_.erase(table_it);
    }
    
    return invalidated_count;
}

void QueryCache::clear() {
    cache_data_.clear();
    access_order_.clear();
    table_to_queries_.clear();
}

void QueryCache::evict_lru() {
    if (access_order_.empty()) {
        return;
    }
    
    // Evict least recently used (first in order)
    std::string lru_key = access_order_.front();
    access_order_.erase(access_order_.begin());
    
    auto it = cache_data_.find(lru_key);
    if (it != cache_data_.end()) {
        // Remove from table tracking
        for (const auto& table : it->second.affected_tables) {
            auto table_it = table_to_queries_.find(table);
            if (table_it != table_to_queries_.end()) {
                table_it->second.erase(lru_key);
                if (table_it->second.empty()) {
                    table_to_queries_.erase(table_it);
                }
            }
        }
        
        cache_data_.erase(it);
        stats_.total_evictions++;
    }
}

void QueryCache::remove_expired_entries() {
    std::vector<std::string> expired_keys;
    
    for (auto& [key, entry] : cache_data_) {
        if (entry.is_expired(ttl_seconds_)) {
            expired_keys.push_back(key);
        }
    }
    
    for (const auto& key : expired_keys) {
        invalidate(key);
        // Note: This is inefficient - should track by query key instead
    }
}

size_t QueryCache::estimate_result_size(const QueryResult* result) const {
    if (!result) return 0;
    
    // Estimate based on row count and column count
    // Assumption: average 50 bytes per cell (strings, numbers, etc.)
    size_t rows = result->row_count();
    size_t cols = result->column_count();
    size_t avg_cell_size = 50;
    
    return rows * cols * avg_cell_size;
}

QueryCache::Statistics QueryCache::get_statistics() const {
    return stats_;
}

}  // namespace lyradb
