#pragma once

#include "query_result.h"
#include <memory>
#include <unordered_map>
#include <string>
#include <chrono>
#include <functional>
#include <set>

namespace lyradb {

/**
 * @class QueryCache
 * @brief LRU query result cache with TTL-based expiration and invalidation
 * 
 * Caches SELECT query results to avoid expensive re-computation.
 * Features:
 * - Hash-based key from query text
 * - TTL-based automatic expiration (default 5 minutes)
 * - LRU eviction when cache is full
 * - Statistics tracking (hit ratio, evictions, memory usage)
 * - Selective invalidation on data mutations
 * 
 * Usage:
 *   QueryCache cache(max_entries=1000, ttl_seconds=300);
 *   
 *   // Get or compute
 *   if (auto cached = cache.get(query_sql)) {
 *       return cached;  // Cache hit
 *   }
 *   auto result = execute_query(query_sql);
 *   cache.put(query_sql, result, affected_tables);
 *   
 *   // Invalidate on mutations
 *   cache.invalidate("employees");
 */
class QueryCache {
public:
    /**
     * Create a query result cache
     * @param max_entries Maximum number of cached queries (default 1000)
     * @param ttl_seconds Time-to-live for cached entries in seconds (default 300)
     * @param max_bytes Maximum total memory usage in bytes (default 100MB)
     */
    QueryCache(size_t max_entries = 1000, 
               int64_t ttl_seconds = 300,
               size_t max_bytes = 100 * 1024 * 1024);
    
    ~QueryCache() = default;
    
    // Non-copyable
    QueryCache(const QueryCache&) = delete;
    QueryCache& operator=(const QueryCache&) = delete;
    
    /**
     * Get cached result for a query
     * @param query_sql SQL query string
     * @return Cached result if exists and not expired, nullptr otherwise
     */
    std::shared_ptr<QueryResult> get(const std::string& query_sql);
    
    /**
     * Store query result in cache
     * @param query_sql SQL query string
     * @param result Query result to cache
     * @param affected_tables Tables referenced in this query (for invalidation)
     */
    void put(const std::string& query_sql,
             std::shared_ptr<QueryResult> result,
             const std::set<std::string>& affected_tables = {});
    
    /**
     * Invalidate all cached queries that reference a table
     * @param table_name Table name to invalidate
     * @return Number of cache entries invalidated
     */
    size_t invalidate(const std::string& table_name);
    
    /**
     * Clear all cache entries
     */
    void clear();
    
    /**
     * Get cache statistics
     */
    struct Statistics {
        size_t total_hits = 0;
        size_t total_misses = 0;
        size_t total_evictions = 0;
        size_t current_entries = 0;
        size_t current_bytes_used = 0;
        float hit_ratio() const {
            size_t total = total_hits + total_misses;
            return total > 0 ? (float)total_hits / total : 0.0f;
        }
    };
    
    /**
     * Get current cache statistics
     */
    Statistics get_statistics() const;
    
    /**
     * Set new TTL for cache entries
     */
    void set_ttl(int64_t seconds) { ttl_seconds_ = seconds; }
    
    /**
     * Set maximum cache entries
     */
    void set_max_entries(size_t max) { max_entries_ = max; }
    
    /**
     * Set maximum memory usage
     */
    void set_max_bytes(size_t max) { max_bytes_ = max; }
    
    /**
     * Enable/disable caching
     */
    void enable(bool enabled) { enabled_ = enabled; }
    
    /**
     * Check if caching is enabled
     */
    bool is_enabled() const { return enabled_; }

private:
    struct CacheEntry {
        std::shared_ptr<QueryResult> result;
        std::chrono::steady_clock::time_point created_at;
        std::set<std::string> affected_tables;
        size_t bytes_used = 0;
        
        bool is_expired(int64_t ttl_seconds) const {
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::steady_clock::now() - created_at
            );
            return elapsed.count() > ttl_seconds;
        }
    };
    
    using CacheMap = std::unordered_map<std::string, CacheEntry>;
    using AccessOrder = std::vector<std::string>;  // LRU order
    
    // Helper functions
    std::string compute_cache_key(const std::string& query_sql) const;
    size_t estimate_result_size(const QueryResult* result) const;
    void evict_lru();
    void remove_expired_entries();
    
    // Cache storage
    CacheMap cache_data_;
    AccessOrder access_order_;  // For LRU tracking
    
    // Configuration
    size_t max_entries_;
    int64_t ttl_seconds_;
    size_t max_bytes_;
    bool enabled_ = true;
    
    // Statistics
    mutable Statistics stats_;
    
    // Invalidation tracking: table_name -> set of queries using it
    std::unordered_map<std::string, std::set<std::string>> table_to_queries_;
};

}  // namespace lyradb
